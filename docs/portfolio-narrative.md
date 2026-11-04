# Interview narrative (senior firmware / embedded)

Use this repo to tell a **single coherent story** in ~3 minutes:

## Problem

Deploy sensor data from edge devices with **credential rotation**, **signed firmware**, and **recoverable updates** — without exposing plaintext secrets on UART or leaving BLE open in the field.

## Design choices

| Topic | What you did | Why it matters |
|-------|----------------|----------------|
| RTOS | Zephyr preemptive threads + dedicated connectivity thread | Clear isolation between real-time sampling and bursty network I/O |
| Connectivity | BLE for provisioning, Wi-Fi for MQTT backhaul | Matches consumer/industrial onboarding patterns |
| Security | MCUboot + imgtool signatures | Root of trust for OTA; ties to supply-chain signing |
| FOTA | Dual slot + confirm-after-boot | Prevents bricked fleet from bad OTA |
| Power | `CONFIG_PM` + app-level sleep when cloud idle | Shows you think about battery / thermal constraints |
| Diagnostics | Structured logging + optional shell | Field debuggability without JTAG |

## Demo script (bench)

1. Show boot log: MCUboot version, confirmed image flag.
2. BLE advertise → provision (or show hard-coded Wi-Fi for lab).
3. MQTT publish JSON telemetry (extend `mqtt_client_app.c`).
4. Trigger test OTA with a bumped `APP_VERSION` string; reboot into new slot; confirm image.

## Stretch goals (prioritize one for depth)

- **Threat model doc**: STRIDE on BLE provisioning window.
- **CI**: `west build` on push with `native_sim` or board QEMU where possible.
- **Metrics**: heap watermark, MQTT reconnect count in `diagnostics.c`.

## Resume bullet (example)

> Built Zephyr-based secure edge firmware (MCUboot, signed FOTA, MQTT/TLS, BLE provisioning) on nRF5340 + Wi-Fi, with modular drivers and power-aware connectivity stack.
