/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SECURE_IOT_EDGE_BLE_PROVISION_H_
#define SECURE_IOT_EDGE_BLE_PROVISION_H_

#include <stdbool.h>

int ble_provision_start(void);
void ble_provision_stop(void);
void ble_provision_reopen_window(void);
bool ble_provision_window_active(void);

#endif /* SECURE_IOT_EDGE_BLE_PROVISION_H_ */