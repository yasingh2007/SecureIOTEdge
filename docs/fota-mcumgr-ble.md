# FOTA: self-test + MCUmgr over BLE

## Self-test before confirm

After MCUboot boots a **test** image (post-OTA), `fota_manager` waits until `fota_self_test_run()` passes, then calls `boot_write_img_confirmed()`.

| Check | Kconfig |
|-------|---------|
| Minimum uptime | `SECURE_IOT_EDGE_FOTA_SELF_TEST_MIN_UPTIME_MS` (default 15 s) |
| Wi-Fi link up | `SECURE_IOT_EDGE_FOTA_SELF_TEST_REQUIRE_WIFI` |
| MQTT connected | `SECURE_IOT_EDGE_FOTA_SELF_TEST_REQUIRE_MQTT` |
| Telemetry JSON | `SECURE_IOT_EDGE_FOTA_SELF_TEST_REQUIRE_SENSOR` |

Manual confirm via MQTT `{"cmd":"fota_confirm"}` runs the same tests unless `SECURE_IOT_EDGE_FOTA_CONFIRM_BYPASS=y`.

Debug: `{"cmd":"self_test"}`.

If self-tests never pass, MCUboot can revert the test image on the next reboot (swap policy dependent).

## MCUmgr SMP over BLE

Enabled in `prj.conf.sample` (`CONFIG_MCUMGR_TRANSPORT_BT`, image/OS/stat groups).

- `fota_smp_bt_init()` registers SMP after `bt_enable()`.
- When the **provision window closes** (timeout, success, or DHCP), the device switches to **MCUmgr connectable advertising** (device name `CONFIG_BT_DEVICE_NAME`).

Provision and MCUmgr share one GATT server; only one connectable advertiser runs at a time.

### Host setup

Install [mcumgr-cli](https://github.com/apache/mynewt-mcumgr-cli) (or `go install` per its README).

### Update workflow

1. Build and sign the image (sysbuild + imgtool / `west sign`).
2. Locate the signed update artifact, often `build/*/zephyr/app_update.bin` or `signed.bin` depending on your build dir.
3. From a machine with BLE:

```bash
# Discover / use advertised name SecureIoTEdge
mcumgr --conntype ble --connstring peer_name='SecureIoTEdge' echo hello

mcumgr --conntype ble --connstring peer_name='SecureIoTEdge' \
  image upload build/secure_iot_edge/zephyr/app_update.bin

mcumgr --conntype ble --connstring peer_name='SecureIoTEdge' image test 0
mcumgr --conntype ble --connstring peer_name='SecureIoTEdge' image confirm 0
mcumgr --conntype ble --connstring peer_name='SecureIoTEdge' reset
```

4. After reboot, connect Wi-Fi and MQTT; wait for self-test + auto confirm, or publish `{"cmd":"self_test"}` / `{"cmd":"fota_confirm"}`.

See `scripts/mcumgr-ble-update.ps1` for a Windows wrapper.

### Flash budget

Dual-slot MCUboot layout is required (`boards/*.overlay`). Keep secondary slot size ≥ signed image.