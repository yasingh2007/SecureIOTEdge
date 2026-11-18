/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include "mqtt_broker.h"

LOG_MODULE_DECLARE(mqtt_app, CONFIG_LOG_DEFAULT_LEVEL);

static int try_ipv4_literal(const char *host, uint16_t port, struct sockaddr_storage *addr)
{
	struct sockaddr_in *sin = (struct sockaddr_in *)addr;
	int ret;

	memset(addr, 0, sizeof(*addr));
	sin->sin_family = AF_INET;
	sin->sin_port = htons(port);

	ret = zsock_inet_pton(AF_INET, host, &sin->sin_addr);
	if (ret == 1) {
		LOG_INF("Broker address (literal IPv4) %s:%u", host, port);
		return 0;
	}

	return -EINVAL;
}

static int try_dns(const char *host, uint16_t port, struct sockaddr_storage *addr)
{
	struct zsock_addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};
	struct zsock_addrinfo *result = NULL;
	char port_str[8];
	int ret;

	snprintf(port_str, sizeof(port_str), "%u", port);

	ret = zsock_getaddrinfo(host, port_str, &hints, &result);
	if (ret != 0 || result == NULL) {
		LOG_ERR("DNS resolve failed for %s (%d)", host, ret);
		return -EHOSTUNREACH;
	}

	if (result->ai_addrlen > sizeof(struct sockaddr_storage)) {
		zsock_freeaddrinfo(result);
		return -EINVAL;
	}

	memset(addr, 0, sizeof(*addr));
	memcpy(addr, result->ai_addr, result->ai_addrlen);

	if (result->ai_family == AF_INET) {
		struct sockaddr_in *sin = (struct sockaddr_in *)addr;

		sin->sin_port = htons(port);
	}

	zsock_freeaddrinfo(result);
	LOG_INF("Broker address (DNS) %s:%u", host, port);
	return 0;
}

int mqtt_broker_resolve(const char *host, uint16_t port, struct sockaddr_storage *addr)
{
	if (host == NULL || addr == NULL || host[0] == '\0') {
		return -EINVAL;
	}

	if (try_ipv4_literal(host, port, addr) == 0) {
		return 0;
	}

	return try_dns(host, port, addr);
}