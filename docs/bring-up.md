# Bring-up checklist

## 1. Workspace

```bash
west init -m <this-repo-url> secure_iot_ws
cd secure_iot_ws
west update
west zephyr-export   # if your environment requires it
```

Install the Zephyr SDK and set `ZEPHYR_BASE` per [Zephyr Getting Started](https://docs.zephyrproject.org/latest/develop/getting_started/index.html).

## 2. Application config

```bash
cp apps/secure_iot_edge/prj.conf.sample apps/secure_iot_edge/prj.conf
```

Enable board-specific drivers (I2C, sensor, Wi-Fi firmware) in `prj.conf` and add a devicetree overlay under `boards/`.

## 3. Flash partitions (MCUboot)

Use a board file that defines `slot0_partition`, `slot1_partition`, and `storage` for settings/FOTA. See `boards/nrf7002dk_nrf5340_cpuapp.overlay` for a starting fragment — **verify offsets against your SoC flash map** before production use.

## 4. Build and flash

```bash
west build -b nrf7002dk_nrf5340_cpuapp apps/secure_iot_edge --sysbuild -d build/sie
west flash -d build/sie
```

## 5. Verify secure boot

1. Flash MCUboot + signed app image (sysbuild handles bundling when configured).
2. Confirm boot logs show MCUboot handoff.
3. After OTA test, call `boot_write_img_confirmed()` from `fota_manager.c` once self-tests pass.

## 6. Cloud path

1. Provision Wi-Fi (BLE GATT service — extend `ble_provision.c`).
2. Confirm DHCP and DNS on the interface.
3. Complete MQTT TLS handshake with broker CA in `creds/` (not committed).
