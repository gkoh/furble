#ifndef NIMBLE_STUB_NVS_H_
#define NIMBLE_STUB_NVS_H_
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef uint32_t nvs_handle_t;
typedef enum { NVS_READONLY, NVS_READWRITE } nvs_open_mode_t;
typedef struct {
    size_t used_entries;
    size_t free_entries;
    size_t total_entries;
    size_t namespace_count;
} nvs_stats_t;
static inline esp_err_t nvs_open(const char*, nvs_open_mode_t, nvs_handle_t* out) {
    if (out) *out = 1; return ESP_OK;
}
static inline esp_err_t nvs_open_from_partition(const char*, const char*, nvs_open_mode_t, nvs_handle_t* out) {
    if (out) *out = 1; return ESP_OK;
}
static inline void nvs_close(nvs_handle_t) {}
static inline esp_err_t nvs_erase_all(nvs_handle_t) { return ESP_OK; }
static inline esp_err_t nvs_erase_key(nvs_handle_t, const char*) { return ESP_OK; }
static inline esp_err_t nvs_commit(nvs_handle_t) { return ESP_OK; }
static inline esp_err_t nvs_set_u8(nvs_handle_t, const char*, uint8_t) { return ESP_OK; }
static inline esp_err_t nvs_set_u32(nvs_handle_t, const char*, uint32_t) { return ESP_OK; }
static inline esp_err_t nvs_set_str(nvs_handle_t, const char*, const char*) { return ESP_OK; }
static inline esp_err_t nvs_set_blob(nvs_handle_t, const char*, const void*, size_t) { return ESP_OK; }
static inline esp_err_t nvs_get_i8(nvs_handle_t, const char*, int8_t*) { return ESP_ERR_NVS_NOT_FOUND; }
static inline esp_err_t nvs_get_u8(nvs_handle_t, const char*, uint8_t*) { return ESP_ERR_NVS_NOT_FOUND; }
static inline esp_err_t nvs_get_i16(nvs_handle_t, const char*, int16_t*) { return ESP_ERR_NVS_NOT_FOUND; }
static inline esp_err_t nvs_get_u16(nvs_handle_t, const char*, uint16_t*) { return ESP_ERR_NVS_NOT_FOUND; }
static inline esp_err_t nvs_get_i32(nvs_handle_t, const char*, int32_t*) { return ESP_ERR_NVS_NOT_FOUND; }
static inline esp_err_t nvs_get_u32(nvs_handle_t, const char*, uint32_t*) { return ESP_ERR_NVS_NOT_FOUND; }
static inline esp_err_t nvs_get_i64(nvs_handle_t, const char*, int64_t*) { return ESP_ERR_NVS_NOT_FOUND; }
static inline esp_err_t nvs_get_u64(nvs_handle_t, const char*, uint64_t*) { return ESP_ERR_NVS_NOT_FOUND; }
static inline esp_err_t nvs_get_str(nvs_handle_t, const char*, char*, size_t*) { return ESP_ERR_NVS_NOT_FOUND; }
static inline esp_err_t nvs_get_blob(nvs_handle_t, const char*, void*, size_t*) { return ESP_ERR_NVS_NOT_FOUND; }
static inline esp_err_t nvs_get_stats(const char*, nvs_stats_t* s) {
    if (s) memset(s, 0, sizeof(*s)), s->free_entries = 100;
    return ESP_OK;
}
#ifdef __cplusplus
}
#endif
#endif
