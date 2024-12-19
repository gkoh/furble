#ifndef CANONEOSRP_H
#define CANONEOSRP_H

#include "CanonEOS.h"

namespace Furble {
/**
 * Canon EOS RP.
 */
class CanonEOSRP: public CanonEOS {
 public:
  CanonEOSRP(const void *data, size_t len) : CanonEOS(Type::CANON_EOS_RP, data, len){};
  CanonEOSRP(const NimBLEAdvertisedDevice *pDevice) : CanonEOS(Type::CANON_EOS_RP, pDevice){};

  /**
   * Determine if the advertised BLE device is a Canon EOS RP.
   */
  static bool matches(const NimBLEAdvertisedDevice *pDevice);

 private:
};

}  // namespace Furble
#endif
