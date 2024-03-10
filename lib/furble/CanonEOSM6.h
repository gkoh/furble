#ifndef CANONEOSM6_H
#define CANONEOSM6_H

#include "CanonEOS.h"

namespace Furble {
/**
 * Canon EOS M6.
 */
class CanonEOSM6: public CanonEOS {
 public:
  CanonEOSM6(const void *data, size_t len);
  CanonEOSM6(NimBLEAdvertisedDevice *pDevice);
  ~CanonEOSM6(void);

  /**
   * Determine if the advertised BLE device is a Canon EOS M6.
   */
  static bool matches(NimBLEAdvertisedDevice *pDevice);

 private:
  device_type_t getDeviceType(void);
};

}  // namespace Furble
#endif
