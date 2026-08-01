#ifndef NIMBLE_STUB_NVS_FLASH_H_
#define NIMBLE_STUB_NVS_FLASH_H_
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
static inline esp_err_t nvs_flash_init(void) { return ESP_OK; }
static inline esp_err_t nvs_flash_init_partition(const char*) { return ESP_OK; }
#ifdef __cplusplus
}
#endif
#endif
