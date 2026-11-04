/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app_context.h"
#include "secure_iot_edge/fota_manager.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

extern int ble_provision_start(void);
extern int wifi_manager_start(struct app_context *ctx);
extern int mqtt_client_app_start(struct app_context *ctx);
extern int sensor_hub_init(void);
extern int power_manager_init(void);
extern int diagnostics_init(void);
extern void diagnostics_periodic(void);
extern void power_manager_on_idle(struct app_context *ctx);

static void connectivity_thread(void *p1, void *p2, void *p3)
{
	struct app_context *ctx = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_BLE_PROVISION)
	if (ble_provision_start() != 0) {
		LOG_WRN("BLE provisioning not started");
	}
#endif

#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_WIFI)
	if (wifi_manager_start(ctx) != 0) {
		LOG_INF("Waiting for BLE Wi-Fi credentials");
	}
	k_sem_take(&ctx->network_ready, K_FOREVER);
#endif

#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_MQTT)
	if (mqtt_client_app_start(ctx) != 0) {
		LOG_ERR("MQTT client failed to start");
	}
#endif
}

K_THREAD_STACK_DEFINE(connectivity_stack, 8192);
static struct k_thread connectivity_thread_data;

void app_run(struct app_context *ctx)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_FOTA)
	fota_manager_init(ctx);
#endif
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_SENSORS)
	sensor_hub_init();
#endif
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_POWER_MGMT)
	power_manager_init();
#endif
	diagnostics_init();

	k_thread_create(&connectivity_thread_data, connectivity_stack,
			K_THREAD_STACK_SIZEOF(connectivity_stack), connectivity_thread, ctx, NULL,
			NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
	k_thread_name_set(&connectivity_thread_data, "connectivity");

	LOG_INF("Secure IoT Edge running");

	while (true) {
		diagnostics_periodic();
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_FOTA)
		fota_manager_periodic(ctx);
#endif
		power_manager_on_idle(ctx);
		k_sleep(K_SECONDS(10));
	}
}

int main(void)
{
	struct app_context ctx;

	if (app_context_init(&ctx) != 0) {
		return 0;
	}

	app_run(&ctx);
	return 0;
}
