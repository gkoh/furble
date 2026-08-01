#ifndef NIMBLE_STUB_BLE_GATT_H_
#define NIMBLE_STUB_BLE_GATT_H_

#include <stdint.h>
#include "host/ble_uuid.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ble_gatt_error {
    uint16_t status;
    uint16_t att_handle;
} ble_gatt_error;

typedef struct ble_gatt_svc {
    uint16_t start_handle;
    uint16_t end_handle;
    ble_uuid_any_t uuid;
} ble_gatt_svc;

typedef struct ble_gatt_chr {
    uint16_t def_handle;
    uint16_t val_handle;
    uint8_t properties;
    ble_uuid_any_t uuid;
} ble_gatt_chr;

typedef struct ble_gatt_dsc {
    uint16_t handle;
    ble_uuid_any_t uuid;
} ble_gatt_dsc;

typedef struct ble_gatt_attr {
    uint16_t handle;
    uint16_t offset;
    void* om;
} ble_gatt_attr;

typedef struct ble_gatt_access_ctxt {
    uint8_t op;
    struct ble_gatt_chr* chr;
    struct ble_gatt_dsc* dsc;
    void* om;
} ble_gatt_access_ctxt;

typedef struct ble_gatt_register_ctxt {
    uint8_t op;
} ble_gatt_register_ctxt;

typedef struct ble_gatt_svc_def {
    uint8_t type;
} ble_gatt_svc_def;

#ifdef __cplusplus
}
#endif

#endif
