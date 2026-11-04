/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wifi_mgr, CONFIG_LOG_DEFAULT_LEVEL);

#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_WIFI)

#include <errno.h>
#include <string.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>

#include "app_context.h"
#include "secure_iot_edge/wifi_credentials.h"
#include "secure_iot_edge/wifi_manager.h"

static struct app_context *app_ctx;
static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;

static void signal_network_ready(void)
{
	if (app_ctx != NULL) {
		k_sem_give(&app_ctx->network_ready);
	}
}

static void ipv4_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			 struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	if (mgmt_event == NET_EVENT_IPV4_DHCP_BOUND) {
		LOG_INF("IPv4 address assigned (DHCP)");
		signal_network_ready();
	}
}

static void wifi_mgmt_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			      struct net_if *iface)
{
	ARG_UNUSED(iface);

	if (mgmt_event != NET_EVENT_WIFI_CONNECT_RESULT) {
		return;
	}

	const struct wifi_status *status = (const struct wifi_status *)cb->info;

	if (status == NULL) {
		return;
	}

	if (status->status == WIFI_STATUS_CONNECTED) {
		LOG_INF("Wi-Fi associated");
	} else {
		LOG_WRN("Wi-Fi connect failed (%d)", status->status);
	}
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

int wifi_manager_start(struct app_context *ctx)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_WIFI)
	if (ctx == NULL) {
		return -EINVAL;
	}

	app_ctx = ctx;

	wifi_credentials_init();

	net_mgmt_init_event_callback(&wifi_cb, wifi_mgmt_handler,
				     NET_EVENT_WIFI_CONNECT_RESULT);
	net_mgmt_add_event_callback(&wifi_cb);

	net_mgmt_init_event_callback(&ipv4_cb, ipv4_handler, NET_EVENT_IPV4_DHCP_BOUND);
	net_mgmt_add_event_callback(&ipv4_cb);

	return wifi_manager_connect_stored();
#else
	ARG_UNUSED(ctx);
	return -ENOTSUP;
#endif
}
