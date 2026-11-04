# Firmware over-the-air (FOTA)

This project targets **MCUboot** dual-slot updates on Zephyr using **sysbuild** (`sysbuild.conf`).

## Boot flow

1. ROM loads **MCUboot** (bootloader child image).
2. MCUboot verifies the signature on the candidate slot (RSA/EC256 depending on Kconfig).
3. Application runs from primary slot; downloads land in secondary slot.
4. MCUboot swaps or overwrites on next reboot after `img_mgmt` marks the image pending.

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

## Transport options (portfolio extensions)

| Transport | Zephyr pieces | Demo value |
|-----------|---------------|------------|
| BLE SMP | `CONFIG_MCUMGR`, `CONFIG_BT` | Phone / laptop updates without Wi-Fi |
| HTTP(S) | `CONFIG_MCUMGR_GRP_IMG`, download client | Cloud-triggered updates |
| MQTT command | Custom topic → trigger SMP | Shows end-to-end IoT control plane |

## Confirm-after-boot

`src/fota/fota_manager.c` checks `boot_is_img_confirmed()`. Production pattern:

1. Run connectivity + sensor self-test.
2. Call `boot_write_img_confirmed()`.
3. If tests fail, revert via MCUboot or refuse confirm (image reverts on next boot depending on swap strategy).

Document your rollback story in interviews — reviewers care about **bricked-device avoidance** more than raw download speed.
