#ifndef CANONEOSM6_H
#define CANONEOSM6_H

#include "CanonEOS.h"

namespace Furble {
/**
 * Canon EOS M6.
 */
class CanonEOSM6: public CanonEOS {
 public:
  CanonEOSM6(const void *data, size_t len) : CanonEOS(Type::CANON_EOS_M6, data, len){};
  CanonEOSM6(const NimBLEAdvertisedDevice *pDevice) : CanonEOS(Type::CANON_EOS_M6, pDevice){};

  /**
   * Determine if the advertised BLE device is a Canon EOS M6.
   */
  static bool matches(const NimBLEAdvertisedDevice *pDevice);

 private:
};

}  // namespace Furble
#endif
