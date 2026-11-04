/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include "app_context.h"
#include <zephyr/kernel.h>

int app_context_init(struct app_context *ctx)
{
	if (ctx == NULL) {
		return -EINVAL;
	}

	k_sem_init(&ctx->network_ready, 0, 1);
	k_sem_init(&ctx->mqtt_connected, 0, 1);
	ctx->provisioned = false;
	ctx->fota_pending_confirm = false;

	return 0;
}
