/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SECURE_IOT_EDGE_MQTT_COMMAND_H_
#define SECURE_IOT_EDGE_MQTT_COMMAND_H_

#include <stddef.h>

struct app_context;

void mqtt_command_dispatch(struct app_context *ctx, const char *payload, size_t len);

#endif /* SECURE_IOT_EDGE_MQTT_COMMAND_H_ */