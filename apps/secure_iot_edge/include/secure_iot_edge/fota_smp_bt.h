/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SECURE_IOT_EDGE_FOTA_SMP_BT_H_
#define SECURE_IOT_EDGE_FOTA_SMP_BT_H_

/** Call after bt_enable() when MCUmgr BLE transport is enabled. */
void fota_smp_bt_init(void);

/** Connectable advertising for SMP / mcumgr (after provision window closes). */
int fota_smp_bt_resume_connectable(void);

/** Stop SMP advertising while provision window is active. */
void fota_smp_bt_pause_connectable(void);

#endif /* SECURE_IOT_EDGE_FOTA_SMP_BT_H_ */