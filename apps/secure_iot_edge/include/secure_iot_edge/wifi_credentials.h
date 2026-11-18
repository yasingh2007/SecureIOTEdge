/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SECURE_IOT_EDGE_WIFI_CREDENTIALS_H_
#define SECURE_IOT_EDGE_WIFI_CREDENTIALS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SIE_WIFI_SSID_MAX 32
#define SIE_WIFI_PSK_MAX  64

struct sie_wifi_credentials {
	char ssid[SIE_WIFI_SSID_MAX + 1];
	char psk[SIE_WIFI_PSK_MAX + 1];
	uint8_t ssid_len;
	uint8_t psk_len;
};

int wifi_credentials_init(void);
int wifi_credentials_load(struct sie_wifi_credentials *out);
int wifi_credentials_save(const struct sie_wifi_credentials *in);
bool wifi_credentials_has_stored(void);
int wifi_credentials_clear(void);

#endif /* SECURE_IOT_EDGE_WIFI_CREDENTIALS_H_ */
