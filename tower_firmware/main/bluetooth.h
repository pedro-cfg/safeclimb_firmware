#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "sdkconfig.h"

#ifndef BLUETOOTH_H 
#define BLUETOOTH_H  

#ifdef __cplusplus
extern "C" {
#endif

extern char global_data[1024];
extern uint8_t new_data;
void update_and_notify_data(const uint8_t* value, int size);
void Bluetooth_Init();
void initBluetooth();
void enableBluetooth();
void disableBluetooth();

#ifdef __cplusplus
}
#endif

#endif
