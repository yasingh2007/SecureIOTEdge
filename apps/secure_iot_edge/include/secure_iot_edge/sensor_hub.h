/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SECURE_IOT_EDGE_SENSOR_HUB_H_
#define SECURE_IOT_EDGE_SENSOR_HUB_H_

#include <stddef.h>

int sensor_hub_init(void);
int sensor_hub_format_telemetry(char *buf, size_t buflen);

#endif /* SECURE_IOT_EDGE_SENSOR_HUB_H_ */
