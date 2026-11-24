/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "app_context.h"
#include "secure_iot_edge/fota_self_test.h"
#include "secure_iot_edge/sensor_hub.h"
#include "secure_iot_edge/wifi_manager.h"

LOG_MODULE_REGISTER(fota_test, CONFIG_LOG_DEFAULT_LEVEL);

static uint32_t last_fail_mask;

#define FAIL_WIFI   BIT(0)
#define FAIL_MQTT   BIT(1)
#define FAIL_SENSOR BIT(2)
#define FAIL_UPTIME BIT(3)

static int check_wifi(void)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_FOTA_SELF_TEST_REQUIRE_WIFI)
	if (!IS_ENABLED(CONFIG_SECURE_IOT_EDGE_WIFI)) {
		return 0;
	}
	if (!wifi_manager_link_is_up()) {
		last_fail_mask |= FAIL_WIFI;
		return -ENOTCONN;
	}
#endif
	return 0;
}

static int check_mqtt(struct app_context *ctx)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_FOTA_SELF_TEST_REQUIRE_MQTT)
	if (!IS_ENABLED(CONFIG_SECURE_IOT_EDGE_MQTT)) {
		return 0;
	}
	if (ctx == NULL || k_sem_count_get(&ctx->mqtt_connected) == 0) {
		last_fail_mask |= FAIL_MQTT;
		return -ENOTCONN;
	}
#endif
	return 0;
}

static int check_sensor(void)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_FOTA_SELF_TEST_REQUIRE_SENSOR)
	char buf[128];
	int n;

	n = sensor_hub_format_telemetry(buf, sizeof(buf));
	if (n <= 0 || strstr(buf, "\"app\"") == NULL) {
		last_fail_mask |= FAIL_SENSOR;
		return -EIO;
	}
#endif
	return 0;
}

static int check_uptime(void)
{
	if (k_uptime_get() < (int64_t)CONFIG_SECURE_IOT_EDGE_FOTA_SELF_TEST_MIN_UPTIME_MS) {
		last_fail_mask |= FAIL_UPTIME;
		return -EAGAIN;
	}
	return 0;
}

int fota_self_test_run(struct app_context *ctx)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_FOTA_SELF_TEST)
	int err;

	last_fail_mask = 0;

	err = check_uptime();
	if (err != 0) {
		LOG_WRN("Self-test: uptime too low");
		return err;
	}

	err = check_wifi();
	if (err != 0) {
		LOG_WRN("Self-test: Wi-Fi link down");
		return err;
	}

	err = check_mqtt(ctx);
	if (err != 0) {
		LOG_WRN("Self-test: MQTT not connected");
		return err;
	}

	err = check_sensor();
	if (err != 0) {
		LOG_WRN("Self-test: telemetry format failed");
		return err;
	}

	LOG_INF("FOTA self-test passed (app %s)", CONFIG_SECURE_IOT_EDGE_APP_VERSION);
	return 0;
#else
	ARG_UNUSED(ctx);
	return 0;
#endif
}

void fota_self_test_log_summary(void)
{
	if (last_fail_mask == 0U) {
		return;
	}

	LOG_WRN("Self-test fail mask: wifi=%d mqtt=%d sensor=%d uptime=%d",
		(last_fail_mask & FAIL_WIFI) != 0U, (last_fail_mask & FAIL_MQTT) != 0U,
		(last_fail_mask & FAIL_SENSOR) != 0U, (last_fail_mask & FAIL_UPTIME) != 0U);
}