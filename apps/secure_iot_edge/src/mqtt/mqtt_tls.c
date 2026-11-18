/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/tls_credentials.h>

#include "mqtt_tls.h"

LOG_MODULE_DECLARE(mqtt_app, CONFIG_LOG_DEFAULT_LEVEL);

#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_MQTT_TLS)

static const unsigned char mqtt_tls_ca_cert[] = {
#include "mqtt_ca_cert.inc"
};

static sec_tag_t sec_tag_list[] = { CONFIG_SECURE_IOT_EDGE_MQTT_TLS_SEC_TAG };
static struct mqtt_sec_config tls_cfg;
static bool tls_ready;
static char tls_hostname[128];

int mqtt_tls_init(void)
{
	int err;

	if (tls_ready) {
		return 0;
	}

	if (sizeof(mqtt_tls_ca_cert) < 10) {
		LOG_ERR("Broker CA too small — copy certs/ca.pem.example to certs/ca.pem");
		return -ENOENT;
	}

	err = tls_credential_add(CONFIG_SECURE_IOT_EDGE_MQTT_TLS_SEC_TAG, TLS_CREDENTIAL_CA,
				 mqtt_tls_ca_cert, sizeof(mqtt_tls_ca_cert));
	if (err < 0) {
		LOG_ERR("tls_credential_add failed (%d)", err);
		return err;
	}

	strncpy(tls_hostname, CONFIG_SECURE_IOT_EDGE_MQTT_HOST, sizeof(tls_hostname) - 1);
	tls_hostname[sizeof(tls_hostname) - 1] = '\0';

	memset(&tls_cfg, 0, sizeof(tls_cfg));
	tls_cfg.hostname = tls_hostname;
	tls_cfg.sec_tag_list = sec_tag_list;
	tls_cfg.sec_tag_count = ARRAY_SIZE(sec_tag_list);

#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_MQTT_TLS_INSECURE)
	tls_cfg.peer_verify = TLS_PEER_VERIFY_NONE;
	LOG_WRN("MQTT TLS peer verify disabled (lab only)");
#else
	tls_cfg.peer_verify = TLS_PEER_VERIFY_REQUIRED;
#endif

	tls_ready = true;
	LOG_INF("MQTT TLS CA loaded (sec tag %d)", CONFIG_SECURE_IOT_EDGE_MQTT_TLS_SEC_TAG);
	return 0;
}

struct mqtt_sec_config *mqtt_tls_config(void)
{
	if (!tls_ready) {
		return NULL;
	}
	return &tls_cfg;
}

#else

int mqtt_tls_init(void)
{
	return -ENOTSUP;
}

struct mqtt_sec_config *mqtt_tls_config(void)
{
	return NULL;
}

#endif /* CONFIG_SECURE_IOT_EDGE_MQTT_TLS */