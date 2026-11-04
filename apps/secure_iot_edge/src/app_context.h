/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_CONTEXT_H_
#define APP_CONTEXT_H_

#include <zephyr/kernel.h>
#include <stdbool.h>

struct app_context {
	struct k_sem network_ready;
	struct k_sem mqtt_connected;
	bool provisioned;
	bool fota_pending_confirm;
};

int app_context_init(struct app_context *ctx);

#endif /* APP_CONTEXT_H_ */
