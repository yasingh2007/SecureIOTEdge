# BLE Wi-Fi provisioning

Write SSID and PSK to the custom GATT service, then write `0x01` to the control characteristic to save credentials to NVS and start Wi-Fi.

128-bit UUIDs (Nordic-style vendor namespace `nordic-secureiot`):

| Characteristic | UUID (128-bit) |
|----------------|----------------|
| Service | `6e6f7264-6963-7365-6375-7265696f7465` |
| SSID | `6e6f7264-6963-7373-6964-000000000001` |
| PSK | `6e6f7264-6963-7073-6b30-000000000002` |
| Control | `6e6f7264-6963-6374-726c-000000000003` |

Use nRF Connect (mobile) or `btgatt-client` on Linux to write values, then control `0x01`.

After DHCP, the device connects to MQTT and publishes JSON telemetry on `CONFIG_SECURE_IOT_EDGE_MQTT_TOPIC_TELEMETRY`.
