/*
 * SPDX-License-Identifier: Apache-2.0
 * Secure IoT Edge — public application API.
 */

#ifndef SECURE_IOT_EDGE_APP_H_
#define SECURE_IOT_EDGE_APP_H_

#include <stdint.h>
#include <stdbool.h>

struct app_context;

int app_context_init(struct app_context *ctx);
void app_run(struct app_context *ctx);

#endif /* SECURE_IOT_EDGE_APP_H_ */
