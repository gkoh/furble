#ifndef NIMBLE_STUB_FREERTOS_QUEUE_H_
#define NIMBLE_STUB_FREERTOS_QUEUE_H_
#include "freertos/FreeRTOS.h"
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    void* storage;
    UBaseType_t item_size;
    UBaseType_t length;
    UBaseType_t count;
    UBaseType_t head;
} StubQueue;
typedef StubQueue* QueueHandle_t;
static inline QueueHandle_t xQueueCreate(UBaseType_t len, UBaseType_t item_size) {
    StubQueue* q = (StubQueue*)calloc(1, sizeof(StubQueue));
    if (!q) return NULL;
    q->length = len;
    q->item_size = item_size;
    q->storage = calloc(len, item_size ? item_size : 1);
    if (!q->storage) { free(q); return NULL; }
    return q;
}
static inline void vQueueDelete(QueueHandle_t q) {
    if (!q) return;
    free(q->storage);
    free(q);
}
static inline BaseType_t xQueueSend(QueueHandle_t q, const void* item, TickType_t) {
    if (!q || !item || q->count >= q->length) return pdFALSE;
    UBaseType_t idx = (q->head + q->count) % q->length;
    memcpy((char*)q->storage + idx * q->item_size, item, q->item_size);
    q->count++;
    return pdTRUE;
}
static inline BaseType_t xQueueReceive(QueueHandle_t q, void* buf, TickType_t) {
    if (!q || !buf || q->count == 0) return pdFALSE;
    memcpy(buf, (char*)q->storage + q->head * q->item_size, q->item_size);
    q->head = (q->head + 1) % q->length;
    q->count--;
    return pdTRUE;
}
#ifdef __cplusplus
}
#endif
#endif
