/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SECURE_IOT_EDGE_WIFI_MANAGER_H_
#define SECURE_IOT_EDGE_WIFI_MANAGER_H_

struct app_context;

int wifi_manager_start(struct app_context *ctx);
int wifi_manager_connect_stored(void);

#endif /* SECURE_IOT_EDGE_WIFI_MANAGER_H_ */
