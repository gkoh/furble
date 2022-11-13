#ifndef FURBLE_H
#define FURBLE_H

#include <M5ez.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEClient.h>
#include <Preferences.h>
#include <vector>

#define FURBLE_STR "furble"

#ifndef FURBLE_VERSION
#define FURBLE_VERSION = "unknown"
#endif

typedef enum {
  FURBLE_FUJIFILM = 1,
  FURBLE_CANON_EOS_M6 = 2,
  FURBLE_CANON_EOS_RP = 3,
} device_type_t;

#include "CanonEOSM6.h"
#include "CanonEOSRP.h"
#include "Fujifilm.h"

#endif
