#ifndef CANONEOSM6_H
#define CANONEOSM6_H

#include "CanonEOS.h"

namespace Furble {
/**
 * Canon EOS M6.
 */
class CanonEOSM6: public CanonEOS {
 public:
  CanonEOSM6(const void *data, size_t len) : CanonEOS(Type::CANON_EOS_M6, data, len) {};
  CanonEOSM6(const NimBLEAdvertisedDevice *pDevice) : CanonEOS(Type::CANON_EOS_M6, pDevice) {};

  /**
   * Determine if the advertised BLE device is a Canon EOS M6.
   */
  static bool matches(const NimBLEAdvertisedDevice *pDevice);

  void shutterPress(void) override final;
  void shutterRelease(void) override final;
  void focusPress(void) override final;
  void focusRelease(void) override final;
  void updateGeoData(const gps_t &gps, const timesync_t &timesync) override final;

 private:
  static constexpr size_t ADV_DATA_LEN = 21;
  static constexpr uint8_t ID_0 = 0xa9;
  static constexpr uint8_t ID_1 = 0x01;
  static constexpr uint8_t XX_2 = 0x01;
  static constexpr uint8_t XX_3 = 0xc5;
  static constexpr uint8_t XX_4 = 0x32;

  const NimBLEUUID SVC_IDEN_UUID {0x00010000, 0x0000, 0x1000, 0x0000d8492fffa821};
  /** 0xf108 */
  const NimBLEUUID CHR_NAME_UUID {0x00010006, 0x0000, 0x1000, 0x0000d8492fffa821};
  /** 0xf104 */
  const NimBLEUUID CHR_IDEN_UUID {0x0001000a, 0x0000, 0x1000, 0x0000d8492fffa821};

  const NimBLEUUID SVC_MODE_UUID {0x00030000, 0x0000, 0x1000, 0x0000d8492fffa821};
  /** 0xf307 */
  const NimBLEUUID CHR_MODE_UUID {0x00030010, 0x0000, 0x1000, 0x0000d8492fffa821};

  const NimBLEUUID SVC_SHUTTER_UUID {0x00030000, 0x0000, 0x1000, 0x0000d8492fffa821};
  /** 0xf311 */
  const NimBLEUUID CHR_SHUTTER_UUID {0x00030030, 0x0000, 0x1000, 0x0000d8492fffa821};

  static constexpr uint8_t PAIR_ACCEPT = 0x02;
  static constexpr uint8_t PAIR_REJECT = 0x03;
  static constexpr uint8_t MODE_PLAYBACK = 0x01;
  static constexpr uint8_t MODE_SHOOT = 0x02;
  static constexpr uint8_t MODE_WAKE = 0x03;

  volatile uint8_t m_PairResult = 0x00;

  bool _connect(void) override final;

  void pairCallback(NimBLERemoteCharacteristic *, uint8_t *, size_t, bool);

  bool write_prefix(const NimBLEUUID &serviceUUID,
                    const NimBLEUUID &characteristicUUID,
                    const uint8_t prefix,
                    const uint8_t *data,
                    uint16_t length);
};

}  // namespace Furble
#endif
