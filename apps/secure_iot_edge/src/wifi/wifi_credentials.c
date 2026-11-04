/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include "secure_iot_edge/wifi_credentials.h"

LOG_MODULE_REGISTER(wifi_cred, CONFIG_LOG_DEFAULT_LEVEL);

#define SETTINGS_SSID "sie/wifi/ssid"
#define SETTINGS_PSK  "sie/wifi/psk"

static bool stored_valid;

static int settings_set(const char *name, size_t len, settings_read_cb read_cb,
			void *cb_arg)
{
	char buf[SIE_WIFI_PSK_MAX + 1];

	if (len >= sizeof(buf)) {
		return -EINVAL;
	}

	if (read_cb(cb_arg, buf, len) <= 0) {
		return -EINVAL;
	}

	buf[len] = '\0';

	if (strcmp(name, "ssid") == 0) {
		stored_valid = (len > 0);
	} else if (strcmp(name, "psk") == 0) {
		/* psk loaded separately */
	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(sie_wifi, "sie/wifi", settings_set, NULL, NULL, NULL);

int wifi_credentials_init(void)
{
#if IS_ENABLED(CONFIG_SETTINGS)
	int err = settings_subsys_init();

	if (err != 0 && err != -EALREADY) {
		return err;
	}

	err = settings_load();
	if (err != 0) {
		LOG_WRN("settings_load failed (%d)", err);
	}
	return 0;
#else
	return 0;
#endif
}

bool wifi_credentials_has_stored(void)
{
#if IS_ENABLED(CONFIG_SETTINGS)
	return stored_valid;
#else
	return false;
#endif
}

static void fill_from_kconfig(struct sie_wifi_credentials *out)
{
	if (strlen(CONFIG_SECURE_IOT_EDGE_WIFI_SSID) > 0) {
		strncpy(out->ssid, CONFIG_SECURE_IOT_EDGE_WIFI_SSID, sizeof(out->ssid) - 1);
		out->ssid_len = (uint8_t)strlen(out->ssid);
	}
	if (strlen(CONFIG_SECURE_IOT_EDGE_WIFI_PSK) > 0) {
		strncpy(out->psk, CONFIG_SECURE_IOT_EDGE_WIFI_PSK, sizeof(out->psk) - 1);
		out->psk_len = (uint8_t)strlen(out->psk);
	}
}

int wifi_credentials_load(struct sie_wifi_credentials *out)
{
	if (out == NULL) {
		return -EINVAL;
	}

	memset(out, 0, sizeof(*out));

#if IS_ENABLED(CONFIG_SETTINGS)
	if (stored_valid) {
		ssize_t n = settings_get(SETTINGS_SSID, out->ssid, sizeof(out->ssid));

		if (n > 0) {
			out->ssid_len = (uint8_t)n;
			out->ssid[n] = '\0';
		}

		n = settings_get(SETTINGS_PSK, out->psk, sizeof(out->psk));
		if (n > 0) {
			out->psk_len = (uint8_t)n;
			out->psk[n] = '\0';
		}

		if (out->ssid_len > 0) {
			return 0;
		}
	}
#endif

	fill_from_kconfig(out);
	if (out->ssid_len > 0) {
		return 0;
	}

	return -ENOENT;
}

int wifi_credentials_save(const struct sie_wifi_credentials *in)
{
#if IS_ENABLED(CONFIG_SETTINGS)
	int err;

	if (in == NULL || in->ssid_len == 0) {
		return -EINVAL;
	}

	err = settings_save_one(SETTINGS_SSID, in->ssid, in->ssid_len);
	if (err != 0) {
		return err;
	}

	if (in->psk_len > 0) {
		err = settings_save_one(SETTINGS_PSK, in->psk, in->psk_len);
		if (err != 0) {
			return err;
		}
	}

	stored_valid = true;
	LOG_INF("Wi-Fi credentials saved to NVS");
	return 0;
#else
	ARG_UNUSED(in);
	return -ENOTSUP;
#endif
}
