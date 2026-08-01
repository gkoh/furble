#ifndef NIMBLE_STUB_FREERTOS_H_
#define NIMBLE_STUB_FREERTOS_H_
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef uint32_t TickType_t;
#define pdFALSE ((BaseType_t)0)
#define pdTRUE  ((BaseType_t)1)
#define pdPASS  pdTRUE
#define pdFAIL  pdFALSE
#define portMAX_DELAY ((TickType_t)0xffffffffu)
#ifndef configTICK_RATE_HZ
#define configTICK_RATE_HZ 1000
#endif
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
static inline void vTaskDelay(TickType_t ticks) {
    usleep((useconds_t)ticks * 1000u);
}
#ifdef __cplusplus
}
#endif
#endif
