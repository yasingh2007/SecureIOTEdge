# MQTT cloud channel

## Broker address

`CONFIG_SECURE_IOT_EDGE_MQTT_HOST` may be an **IPv4 literal** or a **hostname**. Hostnames are resolved with `getaddrinfo()` after Wi-Fi and DHCP are up (`CONFIG_DNS_RESOLVER=y`).

On connect failure, the client clears the cached address and resolves again.

## TLS

Enable in `prj.conf`:

```ini
CONFIG_SECURE_IOT_EDGE_MQTT_TLS=y
CONFIG_SECURE_IOT_EDGE_MQTT_PORT=8883
CONFIG_TLS_CREDENTIALS=y
CONFIG_MBEDTLS_PEM_CERTIFICATE_FORMAT=y
```

Place the broker CA at `apps/secure_iot_edge/certs/ca.pem` (see `certs/README.md`). The CA is registered at boot via `tls_credential_add()` and used for `MQTT_TRANSPORT_SECURE` with SNI set to the MQTT hostname.

Lab builds with self-signed brokers:

```ini
CONFIG_SECURE_IOT_EDGE_MQTT_TLS_INSECURE=y
```

This sets peer verification to **none** — not for production.

## Topics

| Direction | Kconfig default | Payload |
|-----------|-----------------|---------|
| Publish | `iot/edge/telemetry` | JSON from `sensor_hub_format_telemetry()` |
| Subscribe | `iot/edge/cmd` | JSON commands (see below) |

## Command messages

Publish JSON to the command topic:

```json
{"cmd":"publish_now"}
```

| `cmd` | Action |
|-------|--------|
| `publish_now` | Immediate telemetry publish |
| `reboot` | Cold reset |
| `fota_confirm` | `boot_write_img_confirmed()` |
| `factory_reset` | Erase Wi-Fi credentials in NVS |

Examples with Mosquitto:

```bash
mosquitto_pub -h broker.local -t iot/edge/cmd -m '{"cmd":"publish_now"}'
mosquitto_pub -h broker.local -t iot/edge/cmd -m '{"cmd":"reboot"}'
```