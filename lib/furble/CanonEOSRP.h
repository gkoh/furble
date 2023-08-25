#ifndef CANONEOSRP_H
#define CANONEOSRP_H

#include "CanonEOSRemote.h"

namespace Furble {
/**
 * Canon EOS RP.
 */
class CanonEOSRP: public CanonEOSRemote {
 public:
  CanonEOSRP(const void *data, size_t len) : CanonEOSRemote(data, len) {}
  CanonEOSRP(NimBLEAdvertisedDevice *pDevice) : CanonEOSRemote(pDevice) {}
  ~CanonEOSRP(void) {}

  /**
   * Determine if the advertised BLE device is a Canon EOS RP.
   */
  static bool matches(NimBLEAdvertisedDevice *pDevice);

 private:
  Device::type_t getDeviceType(void);
};
}  // namespace Furble
#endif
