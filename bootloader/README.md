# MCUboot child image

Zephyr **sysbuild** builds MCUboot when `SB_CONFIG_BOOTLOADER_MCUBOOT=y` (see `apps/secure_iot_edge/sysbuild.conf`).

Board-specific MCUboot Kconfig fragments can live here, for example:

- `mcuboot.conf` — logging, signature type, downgrade prevention
- `mcuboot.overlay` — serial recovery pin, optional external flash

Refer to upstream:

- [Zephyr MCUboot](https://docs.zephyrproject.org/latest/samples/sysbuild/with_mcuboot/README.html)
- [MCUboot imgtool](https://docs.mcuboot.com/imgtool.html)

Embed the **public** verification key in MCUboot; sign release images in CI with the private key stored in HSM or GitHub Actions secrets.
