#ifndef CANONEOSRP_H
#define CANONEOSRP_H

#include "CanonEOS.h"

namespace Furble {
/**
 * Canon EOS RP.
 */
class CanonEOSRP: public CanonEOS {
 public:
  CanonEOSRP(const void *data, size_t len) : CanonEOS(data, len){};
  CanonEOSRP(NimBLEAdvertisedDevice *pDevice) : CanonEOS(pDevice){};

  /**
   * Determine if the advertised BLE device is a Canon EOS RP.
   */
  static bool matches(NimBLEAdvertisedDevice *pDevice);

 private:
  device_type_t getDeviceType(void) override;
};

}  // namespace Furble
#endif
