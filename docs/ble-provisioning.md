# BLE Wi-Fi provisioning

Write SSID and PSK to the custom GATT service, then write `0x01` to the control characteristic to save credentials to NVS and start Wi-Fi.

## Provision window

| Kconfig | Behavior |
|---------|----------|
| `SECURE_IOT_EDGE_BLE_PROVISION_WINDOW_SEC` | Advertising stops when the timer expires (default 300 s). |
| `SECURE_IOT_EDGE_BLE_PROVISION_STOP_ON_NETWORK` | Stops advertising when DHCP completes. |
| `SECURE_IOT_EDGE_BLE_PROVISION_ALWAYS` | Advertise even if NVS already has credentials. |

On boot, advertising is **skipped** when valid credentials are already stored (unless `ALWAYS` is set).

GATT writes are **rejected** after the window closes (`ATT_ERR_WRITE_NOT_PERMIT`).

Reopen the window:

- MQTT: `{"cmd":"provision"}`
- MQTT: `{"cmd":"factory_reset"}` (also clears NVS credentials)

## GATT UUIDs

| Characteristic | UUID (128-bit) |
|----------------|----------------|
| Service | `6e6f7264-6963-7365-6375-7265696f7465` |
| SSID | `6e6f7264-6963-7373-6964-000000000001` |
| PSK | `6e6f7264-6963-7073-6b30-000000000002` |
| Control | `6e6f7264-6963-6374-726c-000000000003` |

Use nRF Connect (mobile) or `btgatt-client` on Linux to write values, then control `0x01`.

## Wi-Fi reconnect

After provisioning, `wifi_reconn` retries with exponential backoff (`SECURE_IOT_EDGE_WIFI_RECONNECT_MIN_SEC` … `MAX_SEC`) on connect failures, disconnect events, or IPv4 loss.

After DHCP, the device connects to MQTT and publishes telemetry on `CONFIG_SECURE_IOT_EDGE_MQTT_TOPIC_TELEMETRY`.