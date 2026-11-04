/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>

#include "app_context.h"

LOG_MODULE_REGISTER(power, CONFIG_LOG_DEFAULT_LEVEL);

int power_manager_init(void)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_POWER_MGMT)
	LOG_INF("Power management enabled");
	return 0;
#else
	return -ENOTSUP;
#endif
}

void power_manager_on_idle(struct app_context *ctx)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_POWER_MGMT)
	if (ctx == NULL) {
		return;
	}

	if (k_sem_count_get(&ctx->mqtt_connected) == 0) {
		k_sleep(K_MSEC(250));
	}
#endif
}
