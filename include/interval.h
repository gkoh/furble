#ifndef INTERVAL_H
#define INTERVAL_H

#include <Furble.h>
#include "spinner.h"

#define INTERVAL_DEFAULT_COUNT 10
#define INTERVAL_DEFAULT_COUNT_UNIT SPIN_UNIT_NIL
#define INTERVAL_DEFAULT_SHUTTER 50
#define INTERVAL_DEFAULT_SHUTTER_UNIT SPIN_UNIT_MS
#define INTERVAL_DEFAULT_DELAY 15
#define INTERVAL_DEFAULT_DELAY_UNIT SPIN_UNIT_SEC

struct __attribute__((packed)) interval_t {
  SpinValue count;
  SpinValue delay;
  SpinValue shutter;
};

void remote_interval(Furble::Device *device);

#endif
