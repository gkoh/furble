#ifndef NIMBLE_STUB_BLE_GAP_H_
#define NIMBLE_STUB_BLE_GAP_H_

#include <stdint.h>
#include <stdbool.h>
#include "nimble/ble.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ble_gap_sec_state {
    unsigned encrypted : 1;
    unsigned authenticated : 1;
    unsigned bonded : 1;
    unsigned key_size : 5;
};

struct ble_gap_conn_desc {
    ble_addr_t peer_ota_addr;
    ble_addr_t peer_id_addr;
    uint16_t conn_handle;
    uint16_t conn_itvl;
    uint16_t conn_latency;
    uint16_t supervision_timeout;
    uint8_t role;
    struct ble_gap_sec_state sec_state;
};

struct ble_gap_upd_params {
    uint16_t itvl_min;
    uint16_t itvl_max;
    uint16_t latency;
    uint16_t supervision_timeout;
    uint16_t min_ce_len;
    uint16_t max_ce_len;
};

struct ble_gap_conn_params {
    uint16_t scan_itvl;
    uint16_t scan_window;
    uint16_t itvl_min;
    uint16_t itvl_max;
    uint16_t latency;
    uint16_t supervision_timeout;
    uint16_t min_ce_len;
    uint16_t max_ce_len;
};

struct ble_gap_disc_params {
    uint16_t itvl;
    uint16_t window;
    uint8_t filter_policy;
    uint8_t limited : 1;
    uint8_t passive : 1;
    uint8_t filter_duplicates : 1;
};

struct ble_gap_adv_params {
    uint8_t conn_mode;
    uint8_t disc_mode;
    uint16_t itvl_min;
    uint16_t itvl_max;
    uint8_t channel_map;
    uint8_t filter_policy;
    uint8_t high_duty_cycle : 1;
};

struct ble_gap_ext_adv_params {
    uint8_t dummy;
};

struct ble_gap_event {
    uint8_t type;
    /* opaque payload — headers only pass pointers */
    union {
        void* dummy;
    };
};

struct ble_gap_event_listener {
    void* fn;
    void* arg;
};

struct ble_store_status_event {
    int reason;
};

#ifndef BLE_STORE_EVENT_FULL
#define BLE_STORE_EVENT_FULL 1
#endif
#ifndef BLE_STORE_EVENT_OVERFLOW
#define BLE_STORE_EVENT_OVERFLOW 2
#endif

static inline uint16_t ble_att_mtu(uint16_t conn_handle) {
    (void)conn_handle;
    return 23;
}

#ifdef __cplusplus
}
#endif

#endif

/* Client headers reference GATT types without including ble_gatt.h */
#include "host/ble_gatt.h"

#ifndef BLE_GAP_INITIAL_CONN_ITVL_MIN
#define BLE_GAP_INITIAL_CONN_ITVL_MIN 24
#endif
#ifndef BLE_GAP_INITIAL_CONN_ITVL_MAX
#define BLE_GAP_INITIAL_CONN_ITVL_MAX 40
#endif
#ifndef BLE_GAP_INITIAL_SUPERVISION_TIMEOUT
#define BLE_GAP_INITIAL_SUPERVISION_TIMEOUT 256
#endif
#ifndef BLE_GAP_INITIAL_CONN_LATENCY
#define BLE_GAP_INITIAL_CONN_LATENCY 0
#endif
