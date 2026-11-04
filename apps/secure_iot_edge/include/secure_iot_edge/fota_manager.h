/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SECURE_IOT_EDGE_FOTA_MANAGER_H_
#define SECURE_IOT_EDGE_FOTA_MANAGER_H_

struct app_context;

int fota_manager_init(struct app_context *ctx);
void fota_manager_periodic(struct app_context *ctx);

#endif /* SECURE_IOT_EDGE_FOTA_MANAGER_H_ */
