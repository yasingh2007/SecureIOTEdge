# Secure IoT Edge

Portfolio-grade embedded firmware for a **secure IoT edge device** on **Zephyr RTOS**: dual-radio connectivity, MQTT telemetry, **MCUboot** secure boot, signed **FOTA**, sensors, power management, and structured diagnostics.

## Target hardware (reference)

| Board | Role |
|-------|------|
| [nRF7002 DK](https://www.nordicsemi.com/Products/Development-hardware/nRF7002-DK) (nRF5340 + Wi-Fi 6) | Primary reference: BLE provisioning + Wi-Fi backhaul |
| Any Zephyr board with `mcuboot` + flash partition support | Alternate bring-up |

Adjust `board` in `west build` and overlays under `boards/` for your kit.

## System architecture

High-level layers from hardware through cloud integration:

<p align="center">
  <img src="docs/images/architecture.svg" alt="Secure IoT Edge system architecture: cloud, connectivity, application modules, MCUboot, and Zephyr hardware layers" width="920"/>
</p>

Logical view of how edge subsystems connect to connectivity, security, and cloud:

```mermaid
flowchart TB
  subgraph edge["Edge device"]
    APP[Application tasks]
    SENS[Sensor abstraction]
    PWR[Power policy]
    LOG[Logging / diagnostics]
    APP --> SENS
    APP --> PWR
    APP --> LOG
  end

  subgraph net["Connectivity"]
    BLE[BLE GATT / provisioning]
    WIFI[Wi-Fi station]
    MQTT[MQTT client]
    BLE --> WIFI
    WIFI --> MQTT
  end

  subgraph sec["Security & updates"]
    MCUB[MCUboot verify]
    IMG[Image manager / swap]
    MCUB --> IMG
  end

  APP --> net
  APP --> sec
  MQTT --> CLOUD[(MQTT broker / cloud)]
```

<details>
<summary>Expanded Mermaid (modules & layers)</summary>

```mermaid
flowchart TB
  subgraph cloud["Cloud"]
    BROKER[MQTT broker]
    OTA[OTA artifacts]
  end

  subgraph conn["Connectivity"]
    BLE[BLE GATT provisioning]
    WIFI[Wi-Fi + DHCP]
    MQTT[MQTT client]
    BLE --> WIFI --> MQTT
  end

  subgraph app["Application"]
    MAIN[main / connectivity thread]
    SENS[sensor_hub]
    FOTA[fota_manager]
    CRED[wifi_credentials / NVS]
    DIAG[diagnostics]
    PWR[power_manager]
  end

  subgraph sec["Secure boot"]
    MCUB[MCUboot]
    S0[slot0]
    S1[slot1]
    MCUB --> S0
    MCUB --> S1
  end

  subgraph plat["Zephyr + hardware"]
    ZEPH[RTOS · net · settings]
    HW[nRF5340 · Wi-Fi · sensors]
  end

  MQTT --> BROKER
  OTA -.-> S1
  MAIN --> conn
  MAIN --> SENS
  MAIN --> FOTA
  CRED --> BLE
  CRED --> WIFI
  app --> plat
  sec --> plat
  conn --> cloud
```

</details>

## How it works

End-to-end behavior from cold boot through steady-state operation:

<p align="center">
  <img src="docs/images/runtime-flow.svg" alt="Runtime flow: power on, app start, BLE provisioning, Wi-Fi, MQTT, FOTA confirm, steady state" width="920"/>
</p>

<p align="center">
  <img src="docs/images/secure-boot-ota.svg" alt="MCUboot flash partitions and signed OTA update path" width="920"/>
</p>

<details>
<summary>Sequence diagram (provisioning → cloud)</summary>

```mermaid
sequenceDiagram
  autonumber
  participant Phone as Provisioner (BLE)
  participant BLE as ble_provision
  participant NVS as wifi_credentials
  participant WiFi as wifi_manager
  participant MQTT as mqtt_client
  participant Cloud as MQTT broker
  participant FOTA as fota_manager

  Note over BLE,FOTA: Boot: MCUboot → app main()
  BLE->>Phone: Advertise GATT service
  Phone->>BLE: Write SSID / PSK / control 0x01
  BLE->>NVS: settings_save
  BLE->>WiFi: connect_stored()
  WiFi->>WiFi: Associate + DHCP
  WiFi->>MQTT: network_ready
  MQTT->>Cloud: CONNECT + SUBSCRIBE cmd topic
  loop Every publish interval
    MQTT->>Cloud: PUBLISH telemetry JSON
  end
  Cloud-->>MQTT: Command messages
  MQTT->>FOTA: mqtt_connected sem
  FOTA->>FOTA: boot_write_img_confirmed()
```

</details>

## Features

| Area | Implementation |
|------|----------------|
| RTOS | Zephyr threads, work queues, `k_timer` |
| BLE | Peripheral GAP/GATT for Wi-Fi credential delivery (see `src/ble/`) |
| Wi-Fi | Station mode, reconnect policy (`src/wifi/`) |
| MQTT | TLS-capable client, telemetry + command topics (`src/mqtt/`) |
| Secure boot | MCUboot in secondary slot layout (`bootloader/` + `sysbuild.conf`) |
| FOTA | MCUboot image confirm after MQTT check-in (`src/fota/`) |
| Sensors | BME680 via devicetree or synthetic JSON fallback (`src/sensors/`) |
| Power | Idle backoff when cloud disconnected (`src/power/`) |
| Diagnostics | Heartbeat logging + optional shell (`src/diag/`) |

Runtime flow: **BLE GATT → NVS credentials → Wi-Fi/DHCP → MQTT telemetry + commands → confirm OTA image**. Details: [docs/ble-provisioning.md](docs/ble-provisioning.md).

## Repository layout

```
SecureIOTEdge/
├── west.yml                 # West manifest (Zephyr + MCUboot)
├── apps/secure_iot_edge/    # Main Zephyr application
├── boards/                  # Board overlays & devicetree fragments
├── bootloader/              # MCUboot child image notes & Kconfig fragments
├── docs/                    # Deep dives (FOTA, threat model, bring-up)
└── scripts/                 # Build, sign, and flash helpers
```

## Prerequisites

- [West](https://docs.zephyrproject.org/latest/develop/west/index.html)
- Zephyr SDK (see Zephyr doc for your OS)
- Python 3.10+
- For signing: `imgtool` (from MCUboot, installed via pip or Zephyr env)

## Quick start

```bash
cd SecureIOTEdge
west init -l .
west update

# Build application + MCUboot (sysbuild)
west build -b nrf7002dk_nrf5340_cpuapp apps/secure_iot_edge --sysbuild
west flash
```

Copy `apps/secure_iot_edge/prj.conf.sample` to `prj.conf` and set Wi-Fi/MQTT credentials via Kconfig or `prj.conf` (never commit secrets).

## Configuration

- **`prj.conf`**: enable subsystems (`CONFIG_SECURE_IOT_EDGE_*`).
- **`boards/*.overlay`**: pins, partitions, sensor nodes.
- **`sysbuild.conf`**: MCUboot as bootloader child image.

See [docs/bring-up.md](docs/bring-up.md) and [docs/fota.md](docs/fota.md).

## Security notes (portfolio talking points)

- Verified boot chain: ROM → MCUboot (signature check) → signed application slot.
- Update images signed offline; public key embedded in MCUboot.
- TLS to MQTT broker; optional certificate pinning via `CONFIG_MBEDTLS_*`.
- Minimal attack surface: BLE provisioning window, credential storage in settings/NVS.

## License

Apache-2.0 — suitable for public portfolio repos; replace third-party stack licenses per Zephyr/Nordic terms when shipping binaries.
