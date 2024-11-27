
set(MAIN_SRCS
    "main.cpp" "bluetooth.c" "BluetoothManager.cpp" "LoraManager.cpp" "LoraPackage.cpp" "ADC.cpp" "Temperature.c" "WIFI.cpp"
    "networking/wifi/app_wifi.c"
    "networking/mqtt/subscription_manager.c"
    "networking/mqtt/core_mqtt_agent_manager.c"
    "networking/mqtt/core_mqtt_agent_manager_events.c"
)

set(MAIN_INCLUDE_DIRS
    "."
    "networking/wifi"
    "networking/mqtt"
)

set(MAIN_REQUIRES
    qrcode
    wifi_provisioning
    coreMQTT
    coreMQTT-Agent
    corePKCS11
    coreJSON
    backoffAlgorithm
    esp_secure_cert_mgr
    aws-iot-core-mqtt-file-streams-embedded-c
    FreeRTOS-Libraries-Integration-Tests
    unity
    driver
)

idf_component_register(
    SRCS
        ${MAIN_SRCS}
    INCLUDE_DIRS
        ${MAIN_INCLUDE_DIRS}
    REQUIRES
        ${MAIN_REQUIRES}
)

# Root Certificate
target_add_binary_data(${COMPONENT_TARGET} "certs/root_cert_auth.crt" TEXT)