#ifndef NIMBLE_STUB_NPL_H_
#define NIMBLE_STUB_NPL_H_
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef uint32_t ble_npl_time_t;
typedef struct ble_npl_sem { int dummy; } ble_npl_sem;
typedef struct ble_npl_event { int dummy; } ble_npl_event;
typedef struct ble_npl_callout { int dummy; } ble_npl_callout;
static inline ble_npl_time_t ble_npl_time_ticks_to_ms(ble_npl_time_t t) { return t; }
#ifdef __cplusplus
}
#endif
#endif
