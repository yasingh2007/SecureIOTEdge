#!/usr/bin/env bash
# Build Secure IoT Edge with MCUboot (sysbuild)
set -euo pipefail

BOARD="${BOARD:-nrf7002dk_nrf5340_cpuapp}"
BUILD_DIR="${BUILD_DIR:-build/secure_iot_edge}"

west build -b "${BOARD}" apps/secure_iot_edge --sysbuild -d "${BUILD_DIR}" "$@"
