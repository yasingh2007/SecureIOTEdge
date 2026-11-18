/*
 * SPDX-License-Identifier: Apache-2.0
 * MQTT telemetry publish loop and command subscription.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app_context.h"
#include "mqtt_broker.h"
#include "mqtt_tls.h"
#include "secure_iot_edge/mqtt_client_app.h"
#include "secure_iot_edge/mqtt_command.h"
#include "secure_iot_edge/sensor_hub.h"

LOG_MODULE_REGISTER(mqtt_app, CONFIG_LOG_DEFAULT_LEVEL);

#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_MQTT)

#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/random.h>

static struct app_context *app_ctx;
static struct mqtt_client client;
static struct sockaddr_storage broker;
static struct mqtt_utf8 topic_pub;
static struct mqtt_utf8 topic_sub;
static uint8_t rx_buffer[512];
static uint8_t tx_buffer[512];
static uint8_t payload_buf[256];
static char client_id[32];
static bool mqtt_connected;
static bool broker_resolved;
static struct k_work_delayable publish_work;

static void schedule_publish(void)
{
	k_work_schedule(&publish_work,
			K_SECONDS(CONFIG_SECURE_IOT_EDGE_MQTT_PUBLISH_INTERVAL));
}

void mqtt_client_publish_now(void)
{
	k_work_schedule(&publish_work, K_NO_WAIT);
}

static int ensure_broker_addr(void)
{
	int err;

	if (broker_resolved) {
		return 0;
	}

	err = mqtt_broker_resolve(CONFIG_SECURE_IOT_EDGE_MQTT_HOST,
				  CONFIG_SECURE_IOT_EDGE_MQTT_PORT, &broker);
	if (err != 0) {
		return err;
	}

	broker_resolved = true;
	return 0;
}

static void mqtt_evt_handler(struct mqtt_client *const c, const struct mqtt_evt *evt)
{
	switch (evt->type) {
	case MQTT_EVT_CONNACK: {
		if (evt->result != 0) {
			LOG_ERR("MQTT CONNACK error %d", evt->result);
			break;
		}

		mqtt_connected = true;
		if (app_ctx != NULL) {
			k_sem_give(&app_ctx->mqtt_connected);
		}
		LOG_INF("MQTT connected");

		struct mqtt_topic subs = {
			.topic = topic_sub,
			.qos = MQTT_QOS_1_AT_LEAST_ONCE,
		};
		struct mqtt_subscription_list sublist = {
			.list = &subs,
			.count = 1,
			.message_id = sys_rand32_get() & 0xFFFF,
		};

		mqtt_subscribe(c, &sublist);
		schedule_publish();
		break;
	}

	case MQTT_EVT_PUBLISH: {
		const struct mqtt_publish_param *pub = &evt->param.publish;
		uint8_t cmd[192];
		size_t len = pub->message.payload.len;

		if (len >= sizeof(cmd)) {
			len = sizeof(cmd) - 1;
		}
		if (mqtt_read_publish_payload(c, cmd, len) < 0) {
			break;
		}
		mqtt_command_dispatch(app_ctx, (const char *)cmd, len);
		break;
	}

	case MQTT_EVT_DISCONNECT:
		mqtt_connected = false;
		LOG_WRN("MQTT disconnected");
		break;

	default:
		break;
	}
}

static int publish_telemetry(void)
{
	struct mqtt_publish_param param;
	struct mqtt_topic topic;
	int len;

	if (!mqtt_connected) {
		return -ENOTCONN;
	}

	len = sensor_hub_format_telemetry((char *)payload_buf, sizeof(payload_buf));
	if (len < 0) {
		return len;
	}

	topic.topic = topic_pub;
	topic.qos = MQTT_QOS_0_AT_MOST_ONCE;

	param.message.topic = topic;
	param.message.payload.data = payload_buf;
	param.message.payload.len = (uint16_t)len;
	param.message.topic.qos = MQTT_QOS_0_AT_MOST_ONCE;
	param.message_id = sys_rand32_get() & 0xFFFF;
	param.dup_flag = 0;
	param.retain_flag = 0;

	return mqtt_publish(&client, &param);
}

static void publish_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (publish_telemetry() == 0) {
		schedule_publish();
	} else {
		k_work_schedule(&publish_work, K_SECONDS(5));
	}
}

static int mqtt_connect_client(void)
{
	struct mqtt_conn_param param;
	int err;

	if (ensure_broker_addr() != 0) {
		return -EHOSTUNREACH;
	}

	mqtt_client_init(&client);
	client.broker = &broker;
	client.evt_cb = mqtt_evt_handler;
	client.rx_buf = rx_buffer;
	client.rx_buf_size = sizeof(rx_buffer);
	client.tx_buf = tx_buffer;
	client.tx_buf_size = sizeof(tx_buffer);

#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_MQTT_TLS)
	struct mqtt_sec_config *tls = mqtt_tls_config();

	if (tls == NULL) {
		LOG_ERR("MQTT TLS not initialized");
		return -EACCES;
	}

	client.transport.type = MQTT_TRANSPORT_SECURE;
	client.transport.tls.config = tls;
#else
	client.transport.type = MQTT_TRANSPORT_NON_SECURE;
#endif

	param.client_id.utf8 = (uint8_t *)client_id;
	param.client_id.size = strlen(client_id);
	param.keepalive = CONFIG_SECURE_IOT_EDGE_MQTT_KEEPALIVE;

	err = mqtt_connect(&client, &param);
	if (err != 0) {
		LOG_ERR("mqtt_connect failed %d", err);
	}
	return err;
}

static void mqtt_thread_fn(void *p1, void *p2, void *p3)
{
	struct app_context *ctx = p1;
	int err;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	app_ctx = ctx;

#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_MQTT_TLS)
	if (mqtt_tls_init() != 0) {
		LOG_ERR("MQTT TLS setup failed");
		return;
	}
#endif

	topic_pub.utf8 = (uint8_t *)CONFIG_SECURE_IOT_EDGE_MQTT_TOPIC_TELEMETRY;
	topic_pub.size = strlen(CONFIG_SECURE_IOT_EDGE_MQTT_TOPIC_TELEMETRY);
	topic_sub.utf8 = (uint8_t *)CONFIG_SECURE_IOT_EDGE_MQTT_TOPIC_CMD;
	topic_sub.size = strlen(CONFIG_SECURE_IOT_EDGE_MQTT_TOPIC_CMD);

	snprintf(client_id, sizeof(client_id), "%s-%04x",
		 CONFIG_SECURE_IOT_EDGE_MQTT_CLIENT_ID,
		 (unsigned int)(sys_rand32_get() & 0xFFFF));

	k_work_init_delayable(&publish_work, publish_work_handler);

	while (true) {
		if (!mqtt_connected) {
			if (mqtt_connect_client() != 0) {
				broker_resolved = false;
				k_sleep(K_SECONDS(5));
				continue;
			}
		}

		err = mqtt_live(&client);
		if (err != 0 && err != -EAGAIN) {
			mqtt_connected = false;
			mqtt_disconnect(&client, NULL);
			k_sleep(K_SECONDS(3));
			continue;
		}

		err = mqtt_input(&client);
		if (err != 0 && err != -EAGAIN) {
			mqtt_connected = false;
			mqtt_disconnect(&client, NULL);
			k_sleep(K_SECONDS(3));
			continue;
		}

		k_sleep(K_MSEC(200));
	}
}

K_THREAD_STACK_DEFINE(mqtt_stack, 7168);
static struct k_thread mqtt_thread_data;

#endif /* CONFIG_SECURE_IOT_EDGE_MQTT */

int mqtt_client_app_start(struct app_context *ctx)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_MQTT)
	if (ctx == NULL) {
		return -EINVAL;
	}

	k_thread_create(&mqtt_thread_data, mqtt_stack, K_THREAD_STACK_SIZEOF(mqtt_stack),
			mqtt_thread_fn, ctx, NULL, NULL, K_PRIO_COOP(6), 0, K_NO_WAIT);
	k_thread_name_set(&mqtt_thread_data, "mqtt");

	LOG_INF("MQTT target %s:%d (DNS + %s)", CONFIG_SECURE_IOT_EDGE_MQTT_HOST,
		CONFIG_SECURE_IOT_EDGE_MQTT_PORT,
		IS_ENABLED(CONFIG_SECURE_IOT_EDGE_MQTT_TLS) ? "TLS" : "plain");
	return 0;
#else
	ARG_UNUSED(ctx);
	return -ENOTSUP;
#endif
}

#if !IS_ENABLED(CONFIG_SECURE_IOT_EDGE_MQTT)
void mqtt_client_publish_now(void)
{
}
#endif