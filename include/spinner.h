#ifndef SPINNER_H
#define SPINNER_H

enum spin_unit_t {
  SPIN_UNIT_NIL = 0,  // no units
  SPIN_UNIT_INF = 1,  // ignore value, assume infinity
  SPIN_UNIT_MS = 2,   // milliseconds
  SPIN_UNIT_SEC = 3,  // seconds
  SPIN_UNIT_MIN = 4   // minutes
};

struct __attribute__((packed)) SpinValue {
  uint16_t value;
  spin_unit_t unit;
};

void spinner_modify_value(const char *title, bool preset, SpinValue *sv);

std::string sv2str(const SpinValue *sv);
unsigned long sv2ms(const SpinValue *sv);
void ms2hms(unsigned long ms, unsigned int *h, unsigned int *m, unsigned int *s);

#endif
