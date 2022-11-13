#ifndef CANONEOSRP_H
#define CANONEOSRP_H

#include "CanonEOS.h"

namespace Furble {
/**
 * Canon EOS RP.
 */
class CanonEOSRP: public CanonEOS {
 public:
  CanonEOSRP(const void *data, size_t len);
  CanonEOSRP(NimBLEAdvertisedDevice *pDevice);
  ~CanonEOSRP(void);

  /**
   * Determine if the advertised BLE device is a Canon EOS RP.
   */
  static bool matches(NimBLEAdvertisedDevice *pDevice);

  bool connect(NimBLEClient *pClient, ezProgressBar &progress_bar);

 private:
  device_type_t getDeviceType(void);
};

}  // namespace Furble
#endif
