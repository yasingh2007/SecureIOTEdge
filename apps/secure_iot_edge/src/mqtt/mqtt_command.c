/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/data/json.h>
#include <zephyr/sys/reboot.h>

#include "app_context.h"
#include "secure_iot_edge/ble_provision.h"
#include "secure_iot_edge/fota_manager.h"
#include "secure_iot_edge/mqtt_client_app.h"
#include "secure_iot_edge/mqtt_command.h"
#include "secure_iot_edge/wifi_credentials.h"

LOG_MODULE_REGISTER(mqtt_cmd, CONFIG_LOG_DEFAULT_LEVEL);

struct sie_mqtt_cmd_msg {
	const char *cmd;
};

static const struct json_obj_descr cmd_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct sie_mqtt_cmd_msg, cmd, JSON_TOK_STRING),
};

static void handle_cmd(struct app_context *ctx, const char *cmd)
{
	if (cmd == NULL) {
		return;
	}

	if (strcmp(cmd, "reboot") == 0) {
		LOG_INF("Command: cold reboot");
		k_sleep(K_MSEC(100));
		sys_reboot(SYS_REBOOT_COLD);
		return;
	}

	if (strcmp(cmd, "publish_now") == 0) {
		LOG_INF("Command: publish_now");
		mqtt_client_publish_now();
		return;
	}

	if (strcmp(cmd, "fota_confirm") == 0) {
		LOG_INF("Command: fota_confirm");
		if (ctx != NULL) {
			(void)fota_manager_confirm_now(ctx);
		}
		return;
	}

	if (strcmp(cmd, "factory_reset") == 0) {
		LOG_INF("Command: factory_reset (Wi-Fi credentials)");
		if (wifi_credentials_clear() == 0) {
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_BLE_PROVISION)
			ble_provision_reopen_window();
#endif
			LOG_INF("Credentials cleared — reprovision via BLE or MQTT");
		}
		return;
	}

	if (strcmp(cmd, "provision") == 0) {
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_BLE_PROVISION)
		LOG_INF("Command: provision (reopen BLE window)");
		ble_provision_reopen_window();
#else
		LOG_WRN("BLE provisioning disabled");
#endif
		return;
	}

	LOG_WRN("Unknown command: %s", cmd);
}

void mqtt_command_dispatch(struct app_context *ctx, const char *payload, size_t len)
{
	struct sie_mqtt_cmd_msg msg = { 0 };
	char buf[192];
	int64_t decoded;
	char *json;

	if (payload == NULL || len == 0) {
		return;
	}

	if (len >= sizeof(buf)) {
		len = sizeof(buf) - 1;
	}

	memcpy(buf, payload, len);
	buf[len] = '\0';

	json = buf;
	decoded = json_obj_parse(json, len, cmd_descr, ARRAY_SIZE(cmd_descr), &msg);
	if (decoded < 0) {
		LOG_WRN("Invalid command JSON (%lld): %s", (long long)decoded, buf);
		return;
	}

	LOG_INF("MQTT command accepted");
	handle_cmd(ctx, msg.cmd);
}