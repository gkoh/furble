#ifndef NIMBLE_STUB_ESP_BT_H_
#define NIMBLE_STUB_ESP_BT_H_
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    ESP_PWR_LVL_N12 = 0,
    ESP_PWR_LVL_N9  = 1,
    ESP_PWR_LVL_N6  = 2,
    ESP_PWR_LVL_N3  = 3,
    ESP_PWR_LVL_N0  = 4,
    ESP_PWR_LVL_P3  = 5,
    ESP_PWR_LVL_P6  = 6,
    ESP_PWR_LVL_P9  = 7,
} esp_power_level_t;

typedef enum {
    ESP_BLE_PWR_TYPE_CONN_HDL0 = 0,
    ESP_BLE_PWR_TYPE_CONN_HDL1 = 1,
    ESP_BLE_PWR_TYPE_CONN_HDL2 = 2,
    ESP_BLE_PWR_TYPE_CONN_HDL3 = 3,
    ESP_BLE_PWR_TYPE_CONN_HDL4 = 4,
    ESP_BLE_PWR_TYPE_CONN_HDL5 = 5,
    ESP_BLE_PWR_TYPE_CONN_HDL6 = 6,
    ESP_BLE_PWR_TYPE_CONN_HDL7 = 7,
    ESP_BLE_PWR_TYPE_CONN_HDL8 = 8,
    ESP_BLE_PWR_TYPE_ADV = 9,
    ESP_BLE_PWR_TYPE_SCAN = 10,
    ESP_BLE_PWR_TYPE_DEFAULT = 11,
} esp_ble_power_type_t;
#ifdef __cplusplus
}
#endif
#endif
