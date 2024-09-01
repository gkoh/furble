#ifndef FURBLETYPES_H
#define FURBLETYPES_H

#define FURBLE_STR "furble"

extern const char *LOG_TAG;

typedef enum {
  FURBLE_FUJIFILM = 1,
  FURBLE_CANON_EOS_M6 = 2,
  FURBLE_CANON_EOS_RP = 3,
  FURBLE_MOBILE_DEVICE = 4,
} device_type_t;

#endif
