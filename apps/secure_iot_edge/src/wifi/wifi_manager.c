/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wifi_mgr, CONFIG_LOG_DEFAULT_LEVEL);

#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_WIFI)

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/sys/atomic.h>

#include "app_context.h"
#include "secure_iot_edge/wifi_credentials.h"
#include "secure_iot_edge/wifi_manager.h"

#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_BLE_PROVISION) &&                                  \
	IS_ENABLED(CONFIG_SECURE_IOT_EDGE_BLE_PROVISION_STOP_ON_NETWORK)
#include "secure_iot_edge/ble_provision.h"
#endif

static struct app_context *app_ctx;
static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;
static struct k_sem reconnect_work;
static struct k_thread reconnect_thread;
K_THREAD_STACK_DEFINE(reconnect_stack, 2048);

static atomic_t link_up;
static atomic_t wifi_assoc;
static uint32_t backoff_sec;
static bool first_dhcp_signaled;
static bool reconnect_thread_running;

static void signal_network_ready_once(void)
{
	if (app_ctx == NULL || first_dhcp_signaled) {
		return;
	}

	first_dhcp_signaled = true;
	k_sem_give(&app_ctx->network_ready);
}

static void request_reconnect(void)
{
	atomic_set(&link_up, 0);
	while (k_sem_take(&reconnect_work, K_NO_WAIT) == 0) {
	}
	k_sem_give(&reconnect_work);
}

static int connect_with_creds(const struct sie_wifi_credentials *cred)
{
	struct net_if *iface = net_if_get_default();
	struct wifi_connect_req_params params;

	if (iface == NULL || cred->ssid_len == 0) {
		return -EINVAL;
	}

	memset(&params, 0, sizeof(params));
	params.ssid = cred->ssid;
	params.ssid_length = cred->ssid_len;
	params.channel = WIFI_CHANNEL_ANY;

	if (cred->psk_len > 0) {
		params.psk = cred->psk;
		params.psk_length = cred->psk_len;
		params.security = WIFI_SECURITY_TYPE_PSK;
	} else {
		params.security = WIFI_SECURITY_TYPE_NONE;
	}

	LOG_INF("Connecting to \"%.*s\"", cred->ssid_len, cred->ssid);
	return net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
}

static void ipv4_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			   struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	if (mgmt_event == NET_EVENT_IPV4_DHCP_BOUND) {
		atomic_set(&link_up, 1);
		backoff_sec = CONFIG_SECURE_IOT_EDGE_WIFI_RECONNECT_MIN_SEC;
		LOG_INF("IPv4 address assigned (DHCP)");

#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_BLE_PROVISION) &&                                  \
	IS_ENABLED(CONFIG_SECURE_IOT_EDGE_BLE_PROVISION_STOP_ON_NETWORK)
		ble_provision_stop();
#endif

		signal_network_ready_once();
		return;
	}

	if (mgmt_event == NET_EVENT_IPV4_ADDR_DEL) {
		atomic_set(&link_up, 0);
		LOG_WRN("IPv4 address lost");
		request_reconnect();
	}
}

static void wifi_mgmt_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			      struct net_if *iface)
{
	const struct wifi_status *status;

	ARG_UNUSED(iface);

	if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
		status = (const struct wifi_status *)cb->info;
		if (status == NULL) {
			return;
		}

		if (status->status == WIFI_STATUS_CONNECTED) {
			atomic_set(&wifi_assoc, 1);
			LOG_INF("Wi-Fi associated");
		} else {
			atomic_set(&wifi_assoc, 0);
			atomic_set(&link_up, 0);
			LOG_WRN("Wi-Fi connect failed (%d)", status->status);
			backoff_sec = MIN(backoff_sec * 2U,
					  (uint32_t)CONFIG_SECURE_IOT_EDGE_WIFI_RECONNECT_MAX_SEC);
			request_reconnect();
		}
		return;
	}

	if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
		status = (const struct wifi_status *)cb->info;
		atomic_set(&wifi_assoc, 0);
		atomic_set(&link_up, 0);
		LOG_WRN("Wi-Fi disconnected (%d)", status != NULL ? status->status : -1);
		backoff_sec = MIN(backoff_sec * 2U,
				  (uint32_t)CONFIG_SECURE_IOT_EDGE_WIFI_RECONNECT_MAX_SEC);
		request_reconnect();
	}
}

static void reconnect_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (reconnect_thread_running) {
		(void)k_sem_take(&reconnect_work, K_FOREVER);

		while (atomic_get(&link_up) == 0 && reconnect_thread_running) {
			struct sie_wifi_credentials cred;
			uint32_t delay;
			int err;

			delay = backoff_sec;
			if (delay > (uint32_t)CONFIG_SECURE_IOT_EDGE_WIFI_RECONNECT_MAX_SEC) {
				delay = (uint32_t)CONFIG_SECURE_IOT_EDGE_WIFI_RECONNECT_MAX_SEC;
			}

			k_sleep(K_SECONDS(delay));

			err = wifi_credentials_load(&cred);
			if (err != 0) {
				backoff_sec = MIN(backoff_sec * 2U,
						  (uint32_t)CONFIG_SECURE_IOT_EDGE_WIFI_RECONNECT_MAX_SEC);
				continue;
			}

			err = connect_with_creds(&cred);
			if (err != 0) {
				backoff_sec = MIN(backoff_sec * 2U,
						  (uint32_t)CONFIG_SECURE_IOT_EDGE_WIFI_RECONNECT_MAX_SEC);
				continue;
			}

			/* Allow association + DHCP before assessing failure. */
			k_sleep(K_SECONDS(CONFIG_SECURE_IOT_EDGE_WIFI_RECONNECT_MIN_SEC));

			if (atomic_get(&link_up) != 0) {
				backoff_sec = CONFIG_SECURE_IOT_EDGE_WIFI_RECONNECT_MIN_SEC;
				break;
			}

			if (atomic_get(&wifi_assoc) == 0) {
				backoff_sec = MIN(backoff_sec * 2U,
						  (uint32_t)CONFIG_SECURE_IOT_EDGE_WIFI_RECONNECT_MAX_SEC);
			}
		}
	}
}

#endif /* CONFIG_SECURE_IOT_EDGE_WIFI */

int wifi_manager_connect_stored(void)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_WIFI)
	struct sie_wifi_credentials cred;
	int err;

	err = wifi_credentials_load(&cred);
	if (err != 0) {
		return err;
	}

	return connect_with_creds(&cred);
#else
	return -ENOTSUP;
#endif
}

bool wifi_manager_link_is_up(void)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_WIFI)
	return atomic_get(&link_up) != 0;
#else
	return false;
#endif
}

int wifi_manager_start(struct app_context *ctx)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_WIFI)
	int err;

	if (ctx == NULL) {
		return -EINVAL;
	}

	app_ctx = ctx;
	backoff_sec = CONFIG_SECURE_IOT_EDGE_WIFI_RECONNECT_MIN_SEC;
	reconnect_thread_running = true;

	k_sem_init(&reconnect_work, 0, 1);
	wifi_credentials_init();

	net_mgmt_init_event_callback(
		&wifi_cb, wifi_mgmt_handler,
		NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt_add_event_callback(&wifi_cb);

	net_mgmt_init_event_callback(&ipv4_cb, ipv4_handler,
				     NET_EVENT_IPV4_DHCP_BOUND | NET_EVENT_IPV4_ADDR_DEL);
	net_mgmt_add_event_callback(&ipv4_cb);

	k_thread_create(&reconnect_thread, reconnect_stack,
			K_THREAD_STACK_SIZEOF(reconnect_stack), reconnect_thread_fn, NULL,
			NULL, NULL, K_PRIO_COOP(8), 0, K_NO_WAIT);
	k_thread_name_set(&reconnect_thread, "wifi_reconn");

	err = wifi_manager_connect_stored();
	if (err != 0) {
		LOG_INF("No Wi-Fi yet — open BLE provision window or set Kconfig SSID");
		request_reconnect();
		return 0;
	}

	return 0;
#else
	ARG_UNUSED(ctx);
	return -ENOTSUP;
#endif
}