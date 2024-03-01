#ifndef SPINNER_H
#define SPINNER_H

enum spin_unit_t {
  SPIN_UNIT_NIL = 0,
  SPIN_UNIT_MS  = 1,
  SPIN_UNIT_SEC = 2,
  SPIN_UNIT_MIN = 3
};

struct SpinValue {
  uint16_t value;
  spin_unit_t unit;
};

#endif
