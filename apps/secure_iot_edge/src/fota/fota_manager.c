/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app_context.h"
#include "secure_iot_edge/fota_manager.h"

LOG_MODULE_REGISTER(fota, CONFIG_LOG_DEFAULT_LEVEL);

#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_FOTA)

#include <zephyr/dfu/mcuboot.h>

static bool needs_confirm;
static bool confirm_done;

#endif

int fota_manager_init(struct app_context *ctx)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_FOTA)
	if (ctx == NULL) {
		return -EINVAL;
	}

	if (!boot_is_img_confirmed()) {
		needs_confirm = true;
		ctx->fota_pending_confirm = true;
		LOG_WRN("MCUboot test image — confirm after MQTT check-in");
	} else {
		LOG_INF("Application image confirmed");
	}
	return 0;
#else
	ARG_UNUSED(ctx);
	return -ENOTSUP;
#endif
}

void fota_manager_periodic(struct app_context *ctx)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_FOTA)
	int err;

	if (!needs_confirm || confirm_done || ctx == NULL) {
		return;
	}

	if (k_sem_count_get(&ctx->mqtt_connected) == 0) {
		return;
	}

	err = boot_write_img_confirmed();
	if (err != 0) {
		LOG_ERR("Image confirm failed (%d)", err);
		return;
	}

	needs_confirm = false;
	confirm_done = true;
	ctx->fota_pending_confirm = false;
	LOG_INF("OTA image confirmed");
#endif
}
