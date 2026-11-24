/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app_context.h"
#include "secure_iot_edge/fota_manager.h"
#include "secure_iot_edge/fota_self_test.h"

LOG_MODULE_REGISTER(fota, CONFIG_LOG_DEFAULT_LEVEL);

#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_FOTA)

#include <zephyr/dfu/mcuboot.h>

static bool needs_confirm;
static bool confirm_done;
static bool self_test_passed;

static int confirm_image(struct app_context *ctx)
{
	int err;

	err = boot_write_img_confirmed();
	if (err != 0) {
		LOG_ERR("Image confirm failed (%d)", err);
		return err;
	}

	needs_confirm = false;
	confirm_done = true;
	self_test_passed = false;
	if (ctx != NULL) {
		ctx->fota_pending_confirm = false;
	}
	LOG_INF("MCUboot application image confirmed");
	return 0;
}

#endif /* CONFIG_SECURE_IOT_EDGE_FOTA */

int fota_manager_init(struct app_context *ctx)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_FOTA)
	if (ctx == NULL) {
		return -EINVAL;
	}

	self_test_passed = false;

	if (!boot_is_img_confirmed()) {
		needs_confirm = true;
		ctx->fota_pending_confirm = true;
		LOG_WRN("MCUboot test image — confirm after FOTA self-test");
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

	if (!self_test_passed) {
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_FOTA_SELF_TEST)
		err = fota_self_test_run(ctx);
		if (err != 0) {
			fota_self_test_log_summary();
			return;
		}
#else
		if (IS_ENABLED(CONFIG_SECURE_IOT_EDGE_MQTT) &&
		    k_sem_count_get(&ctx->mqtt_connected) == 0) {
			return;
		}
#endif
		self_test_passed = true;
	}

	(void)confirm_image(ctx);
#endif
}

int fota_manager_confirm_now(struct app_context *ctx)
{
#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_FOTA)
	int err;

#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_FOTA_SELF_TEST) &&                                  \
	!IS_ENABLED(CONFIG_SECURE_IOT_EDGE_FOTA_CONFIRM_BYPASS)
	if (!self_test_passed) {
		err = fota_self_test_run(ctx);
		if (err != 0) {
			fota_self_test_log_summary();
			LOG_ERR("Refusing confirm — self-test failed");
			return err;
		}
		self_test_passed = true;
	}
#endif

	return confirm_image(ctx);
#else
	ARG_UNUSED(ctx);
	return -ENOTSUP;
#endif
}