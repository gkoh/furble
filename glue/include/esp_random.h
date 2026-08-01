#ifndef NIMBLE_STUB_ESP_RANDOM_H_
#define NIMBLE_STUB_ESP_RANDOM_H_
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif
static inline uint32_t esp_random(void) {
    return (uint32_t)rand();
}
static inline void esp_fill_random(void* buf, size_t len) {
    uint8_t* p = (uint8_t*)buf;
    for (size_t i = 0; i < len; i++) {
        p[i] = (uint8_t)(rand() & 0xff);
    }
}
#ifdef __cplusplus
}
#endif
#endif
