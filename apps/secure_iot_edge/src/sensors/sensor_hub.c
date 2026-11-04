/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include "secure_iot_edge/sensor_hub.h"

LOG_MODULE_REGISTER(sensors, CONFIG_LOG_DEFAULT_LEVEL);

static const struct device *env_sensor;

int sensor_hub_init(void)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_SENSORS)
	env_sensor = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(bme680));

	if (env_sensor != NULL && device_is_ready(env_sensor)) {
		LOG_INF("Sensor ready: %s", env_sensor->name);
	} else {
		env_sensor = NULL;
		LOG_INF("No BME680 in devicetree — telemetry uses synthetic values");
	}
#endif
	return 0;
}

int sensor_hub_format_telemetry(char *buf, size_t buflen)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_SENSORS)
	double temp = 22.0;
	double hum = 48.0;
	struct sensor_value tval;
	struct sensor_value hval;
	int err;

	if (env_sensor != NULL) {
		err = sensor_sample_fetch(env_sensor);
		if (err == 0) {
			if (sensor_channel_get(env_sensor, SENSOR_CHAN_AMBIENT_TEMP, &tval) == 0) {
				temp = sensor_value_to_double(&tval);
			}
			if (sensor_channel_get(env_sensor, SENSOR_CHAN_HUMIDITY, &hval) == 0) {
				hum = sensor_value_to_double(&hval);
			}
		}
	}

	return snprintf(buf, buflen,
			"{\"app\":\"%s\",\"temp\":%.1f,\"hum\":%.1f,\"uptime\":%llu}",
			CONFIG_SECURE_IOT_EDGE_APP_VERSION, temp, hum,
			(unsigned long long)k_uptime_get() / 1000ULL);
#else
	ARG_UNUSED(buf);
	ARG_UNUSED(buflen);
	return -ENOTSUP;
#endif
}
