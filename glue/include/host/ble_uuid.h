#ifndef NIMBLE_STUB_BLE_UUID_H_
#define NIMBLE_STUB_BLE_UUID_H_

#include <stdint.h>
#include "nimble/ble.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_UUID_TYPE_16  16
#define BLE_UUID_TYPE_32  32
#define BLE_UUID_TYPE_128 128

typedef struct {
    uint8_t type;
} ble_uuid_t;

typedef struct {
    ble_uuid_t u;
    uint16_t value;
} ble_uuid16_t;

typedef struct {
    ble_uuid_t u;
    uint32_t value;
} ble_uuid32_t;

typedef struct {
    ble_uuid_t u;
    uint8_t value[16];
} ble_uuid128_t;

typedef union {
    ble_uuid_t u;
    ble_uuid16_t u16;
    ble_uuid32_t u32;
    ble_uuid128_t u128;
} ble_uuid_any_t;

#ifdef __cplusplus
}
#endif

#endif
