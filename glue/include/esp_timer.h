#ifndef NIMBLE_STUB_ESP_TIMER_H_
#define NIMBLE_STUB_ESP_TIMER_H_
#include <stdint.h>
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif
static inline int64_t esp_timer_get_time(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (int64_t)ts.tv_sec * 1000000LL + (int64_t)ts.tv_nsec / 1000LL;
}
#ifdef __cplusplus
}
#endif
#endif
