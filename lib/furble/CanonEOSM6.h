#ifndef CANONEOSM6_H
#define CANONEOSM6_H

#include "CanonEOSSmartphone.h"

namespace Furble {
/**
 * Canon EOS M6.
 */
class CanonEOSM6: public CanonEOSSmartphone {
 public:
  CanonEOSM6(const void *data, size_t len) : CanonEOSSmartphone(data, len) {}
  CanonEOSM6(NimBLEAdvertisedDevice *pDevice) : CanonEOSSmartphone(pDevice) {}
  ~CanonEOSM6(void) {}

  /**
   * Determine if the advertised BLE device is a Canon EOS M6.
   */
  static bool matches(NimBLEAdvertisedDevice *pDevice);

 private:
  Device::type_t getDeviceType(void);
};

}  // namespace Furble
#endif
