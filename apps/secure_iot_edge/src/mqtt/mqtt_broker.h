/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MQTT_BROKER_H_
#define MQTT_BROKER_H_

#include <stdint.h>
#include <zephyr/net/socket.h>

int mqtt_broker_resolve(const char *host, uint16_t port, struct sockaddr_storage *addr);

#endif /* MQTT_BROKER_H_ */