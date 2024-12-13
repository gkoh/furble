#ifndef INTERVAL_H
#define INTERVAL_H

#include "FurbleSpinValue.h"

namespace Furble {

constexpr SpinValue::nvs_t INTERVAL_DEFAULT_COUNT = {10, SpinValue::UNIT_NIL};
constexpr SpinValue::nvs_t INTERVAL_DEFAULT_DELAY = {15, SpinValue::UNIT_SEC};
constexpr SpinValue::nvs_t INTERVAL_DEFAULT_SHUTTER = {30, SpinValue::UNIT_MS};

typedef struct __attribute__((packed)) {
  SpinValue::nvs_t count;
  SpinValue::nvs_t delay;
  SpinValue::nvs_t shutter;
} interval_t;

}  // namespace Furble

#endif
