# Firmware over-the-air (FOTA)

This project targets **MCUboot** dual-slot updates on Zephyr using **sysbuild** (`sysbuild.conf`).

Primary OTA transport: **MCUmgr SMP over BLE** — see [fota-mcumgr-ble.md](fota-mcumgr-ble.md).

## Boot flow

1. ROM loads **MCUboot** (bootloader child image).
2. MCUboot verifies the signature on the candidate slot (RSA/EC256 depending on Kconfig).
3. Application runs from primary slot; uploads land in secondary slot via SMP image management.
4. MCUboot swaps or overwrites on next reboot after `img_mgmt` marks the image pending.

## Confirm-after-boot (self-test)

`src/fota/fota_manager.c` + `src/fota/fota_self_test.c`:

1. Detect unconfirmed image with `boot_is_img_confirmed()`.
2. Run Wi-Fi / MQTT / telemetry self-tests (configurable).
3. On success, `boot_write_img_confirmed()` — image becomes permanent.
4. On failure, leave unconfirmed so MCUboot can roll back per swap policy.

## Signing (development)

Generate a key pair once (keep private key offline):

```bash
pip install imgtool
imgtool keygen -k keys/root-ec-p256.pem -t ec256
```

Configure MCUboot and application signing in sysbuild/Kconfig per [Zephyr MCUboot guide](https://docs.zephyrproject.org/latest/services/device_mgmt/dfu.html).

Sign a production image:

```bash
west sign -t imgtool -- --key keys/root-ec-p256.pem
```

## Other transports (reference)

| Transport | Zephyr pieces | Notes |
|-----------|---------------|--------|
| BLE SMP | `CONFIG_MCUMGR_TRANSPORT_BT` | **Implemented** — Nordic-friendly |
| HTTP(S) | MCUmgr / download client | Cloud backhaul extension |
| MQTT command | Trigger factory flow | Complements SMP, not a transport |