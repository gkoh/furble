#ifndef CANONEOSR_H
#define CANONEOSR_H

#include "CanonEOS.h"

namespace Furble {
/**
 * Canon EOS R.
 */
class CanonEOSR: public CanonEOS {
 public:
  CanonEOSR(const void *data, size_t len) : CanonEOS(Type::CANON_EOS_R, data, len) {};
  CanonEOSR(const NimBLEAdvertisedDevice *pDevice) : CanonEOS(Type::CANON_EOS_R, pDevice) {};

  /**
   * Determine if the advertised BLE device is a Canon EOS R.
   */
  static bool matches(const NimBLEAdvertisedDevice *pDevice);

  void shutterPress(void) override final;
  void shutterRelease(void) override final;
  void focusPress(void) override final;
  void focusRelease(void) override final;
  void updateGeoData(const gps_t &gps, const timesync_t &timesync) override final;

 private:
  // Location and time message
  typedef struct __attribute__((packed)) _canon_geo_t {
    uint8_t header;               // 0x04
    uint8_t latitude_direction;   // latitude direction "{N|S}"
    float latitude;               // degrees latitude (float32)
    uint8_t longitude_direction;  // longitude direction "{E|W}"
    float longitude;              // degrees longitude (float32)
    uint8_t elevation_sign;       // elevation sign "{-|+}"
    float elevation;              // metres elevation (float32)
    uint32_t timestamp;           // seconds since epoch
  } canon_geo_t;

  // Primary service
  static const NimBLEUUID PRI_SVC_UUID;

  // Name/ID characteristic
  const NimBLEUUID ID_CHR_UUID {0x00050002, 0x0000, 0x1000, 0x0000d8492fffa821};

  // Control characteristic (focus, shutter)
  const NimBLEUUID CTRL_CHR_UUID {0x00050003, 0x0000, 0x1000, 0x0000d8492fffa821};

  // Location service
  const NimBLEUUID GEO_SVC_UUID {0x00040000, 0x0000, 0x1000, 0x0000d8492fffa821};

  // Location characteristic
  const NimBLEUUID GEO_CHR_UUID {0x00040002, 0x0000, 0x1000, 0x0000d8492fffa821};

  // Location indication
  const NimBLEUUID GEO_IND_UUID {0x00040003, 0x0000, 0x1000, 0x0000d8492fffa821};

  static constexpr uint8_t SHUTTER = 0x80;
  static constexpr uint8_t FOCUS = 0x40;
  static constexpr uint8_t CTRL = 0x0c;

  static constexpr uint8_t GEO_REQUEST = 0x03;
  const std::array<uint8_t, 1> GEO_ENABLE = {0x01};
  static constexpr uint8_t GEO_SUCCESS = 0x02;

  NimBLERemoteCharacteristic *m_Control = nullptr;
  NimBLERemoteCharacteristic *m_Geo = nullptr;
  bool m_GeoEnabled = false;

  bool _connect(void) override final;
};

}  // namespace Furble
#endif
