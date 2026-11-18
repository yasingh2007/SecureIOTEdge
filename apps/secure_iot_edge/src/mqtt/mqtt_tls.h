/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MQTT_TLS_H_
#define MQTT_TLS_H_

#include <zephyr/net/mqtt.h>

int mqtt_tls_init(void);
struct mqtt_sec_config *mqtt_tls_config(void);

#endif /* MQTT_TLS_H_ */