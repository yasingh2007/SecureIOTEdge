/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SECURE_IOT_EDGE_MQTT_CLIENT_APP_H_
#define SECURE_IOT_EDGE_MQTT_CLIENT_APP_H_

struct app_context;

int mqtt_client_app_start(struct app_context *ctx);

/** Schedule an immediate telemetry publish (e.g. from MQTT command). */
void mqtt_client_publish_now(void);

#endif /* SECURE_IOT_EDGE_MQTT_CLIENT_APP_H_ */