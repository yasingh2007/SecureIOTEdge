cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(secure_iot_edge)

set(MQTT_CA_PEM "${CMAKE_CURRENT_SOURCE_DIR}/certs/ca.pem")
if(NOT EXISTS "${MQTT_CA_PEM}")
	set(MQTT_CA_PEM "${CMAKE_CURRENT_SOURCE_DIR}/certs/ca.pem.example")
endif()

zephyr_include_directories(${CMAKE_CURRENT_BINARY_DIR}/include/generated)

generate_inc_file(
	app
	${MQTT_CA_PEM}
	${CMAKE_CURRENT_BINARY_DIR}/include/generated/mqtt_ca_cert.inc
)

target_sources(app PRIVATE
  src/main.c
  src/app_context.c
  src/ble/ble_provision.c
  src/wifi/wifi_credentials.c
  src/wifi/wifi_manager.c
  src/mqtt/mqtt_broker.c
  src/mqtt/mqtt_tls.c
  src/mqtt/mqtt_command.c
  src/mqtt/mqtt_client_app.c
  src/fota/fota_manager.c
  src/sensors/sensor_hub.c
  src/power/power_manager.c
  src/diag/diagnostics.c
)

zephyr_include_directories(src include)
