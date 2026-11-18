# MQTT TLS broker CA

The build embeds `certs/ca.pem` into the firmware (falls back to `ca.pem.example`).

1. Export your broker’s CA certificate as PEM, or use a public root if the broker uses a public chain.
2. Copy it to `ca.pem` (do not commit production-specific CAs if they are sensitive).

Lab Mosquitto with a self-signed server cert: use the CA that signed the broker certificate, or enable `CONFIG_SECURE_IOT_EDGE_MQTT_TLS_INSECURE=y` only on a test build.