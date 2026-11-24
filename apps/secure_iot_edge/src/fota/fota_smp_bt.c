/*
 * SPDX-License-Identifier: Apache-2.0
 * MCUmgr SMP over BLE — connectable advertising when provision window is closed.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(fota_smp, CONFIG_LOG_DEFAULT_LEVEL);

#if IS_ENABLED(CONFIG_SECURE_IOT_EDGE_MCUMGR_BLE_OTA)

#include <errno.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>

#include "secure_iot_edge/fota_smp_bt.h"

static bool smp_adv_active;

static const struct bt_data smp_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

void fota_smp_bt_init(void)
{
#if IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_BT_DYNAMIC_REGISTRATION)
	int err = smp_bt_register();

	if (err != 0 && err != -EALREADY) {
		LOG_ERR("smp_bt_register failed (%d)", err);
	}
#endif
	LOG_INF("MCUmgr SMP over BLE ready (use mcumgr with connstring peer_name=%s)",
		CONFIG_BT_DEVICE_NAME);
}

void fota_smp_bt_pause_connectable(void)
{
	if (!smp_adv_active) {
		return;
	}

	(void)bt_le_adv_stop();
	smp_adv_active = false;
}

int fota_smp_bt_resume_connectable(void)
{
	int err;

	if (smp_adv_active) {
		return 0;
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN, smp_ad, ARRAY_SIZE(smp_ad), NULL, 0);
	if (err != 0) {
		LOG_DBG("SMP advertising start %d (may already be active)", err);
		return err;
	}

	smp_adv_active = true;
	LOG_INF("MCUmgr BLE connectable advertising active");
	return 0;
}

#else

void fota_smp_bt_init(void)
{
}

void fota_smp_bt_pause_connectable(void)
{
}

int fota_smp_bt_resume_connectable(void)
{
	return -ENOTSUP;
}

#endif /* CONFIG_SECURE_IOT_EDGE_MCUMGR_BLE_OTA */