# Signed image upload + test + reset over BLE (MCUmgr)
param(
    [string]$Image = "build\secure_iot_edge\zephyr\app_update.bin",
    [string]$PeerName = "SecureIoTEdge"
)

$conn = "peer_name=$PeerName"

mcumgr --conntype ble --connstring $conn echo hello
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

mcumgr --conntype ble --connstring $conn image upload $Image
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

mcumgr --conntype ble --connstring $conn image test 0
mcumgr --conntype ble --connstring $conn reset

Write-Host "Rebooting into test image — watch serial for FOTA self-test"