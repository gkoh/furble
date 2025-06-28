#ifndef CANONEOSREMOTE_H
#define CANONEOSREMOTE_H

#include "CanonEOS.h"

namespace Furble {
/**
 * Canon EOS Remote.
 *
 * Compatible with BR-E1 pairing.
 */
class CanonEOSRemote: public CanonEOS {
 public:
  CanonEOSRemote(const void *data, size_t len) : CanonEOS(Type::CANON_EOS_REMOTE, data, len) {};
  CanonEOSRemote(const NimBLEAdvertisedDevice *pDevice)
      : CanonEOS(Type::CANON_EOS_REMOTE, pDevice) {};

  /**
   * Determine if the advertised BLE device is a Canon EOS pairing a remote.
   */
  static bool matches(const NimBLEAdvertisedDevice *pDevice);

  void shutterPress(void) override final;
  void shutterRelease(void) override final;
  void focusPress(void) override final;
  void focusRelease(void) override final;
  void updateGeoData(const gps_t &gps, const timesync_t &timesync) override final;

 private:
  // Primary service
  static const NimBLEUUID PRI_SVC_UUID;

  // Name/ID characteristic
  const NimBLEUUID ID_CHR_UUID {0x00050002, 0x0000, 0x1000, 0x0000d8492fffa821};

  // Control characteristic (focus, shutter)
  const NimBLEUUID CTRL_CHR_UUID {0x00050003, 0x0000, 0x1000, 0x0000d8492fffa821};

  static constexpr uint8_t SHUTTER = 0x80;
  static constexpr uint8_t FOCUS = 0x40;
  static constexpr uint8_t CTRL = 0x0c;

  NimBLERemoteCharacteristic *m_Control = nullptr;

  bool _connect(void) override final;
};

}  // namespace Furble
#endif
