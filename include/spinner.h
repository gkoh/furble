#ifndef SPINNER_H
#define SPINNER_H

enum spin_unit_t {
  SPIN_UNIT_NIL = 0,
  SPIN_UNIT_MS  = 1,
  SPIN_UNIT_SEC = 2,
  SPIN_UNIT_MIN = 3
};

struct __attribute__((packed)) SpinValue {
  uint16_t value;
  spin_unit_t unit;
};

void spinner_modify_value(const char *title, bool preset, SpinValue *sv);

String sv2str(SpinValue *sv);
unsigned long sv2ms(SpinValue *sv);
void ms2hms(unsigned long ms, unsigned int *h, unsigned int *m, unsigned int *s);

#endif
