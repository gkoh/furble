#ifndef NIMBLE_STUB_ESP_MAC_H_
#define NIMBLE_STUB_ESP_MAC_H_
#include <stdint.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
static inline esp_err_t esp_efuse_mac_get_default(uint8_t* mac) {
    if (!mac) return ESP_ERR_INVALID_ARG;
    mac[0]=0x02; mac[1]=0x00; mac[2]=0x00; mac[3]=0x00; mac[4]=0x00; mac[5]=0x01;
    return ESP_OK;
}
#ifdef __cplusplus
}
#endif
#endif
