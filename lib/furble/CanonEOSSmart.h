#ifndef CANONEOSSMART_H
#define CANONEOSSMART_H

#include "CanonEOS.h"

namespace Furble {
/**
 * Canon EOS Smart Device.
 *
 * eg. M6
 */
class CanonEOSSmart: public CanonEOS {
 public:
  CanonEOSSmart(const void *data, size_t len) : CanonEOS(Type::CANON_EOS_SMART, data, len) {};
  CanonEOSSmart(const NimBLEAdvertisedDevice *pDevice)
      : CanonEOS(Type::CANON_EOS_SMART, pDevice) {};

  /**
   * Determine if the advertised BLE device is a Canon EOS pairing with a smart device.
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

  static const NimBLEUUID PRI_SVC_UUID;
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

  // Location service
  const NimBLEUUID GEO_SVC_UUID {0x00040000, 0x0000, 0x1000, 0x0000d8492fffa821};

  // Location characteristic
  const NimBLEUUID GEO_CHR_UUID {0x00040002, 0x0000, 0x1000, 0x0000d8492fffa821};

  // Location indication
  const NimBLEUUID GEO_IND_UUID {0x00040003, 0x0000, 0x1000, 0x0000d8492fffa821};

  static constexpr uint8_t PAIR_ACCEPT = 0x02;
  static constexpr uint8_t PAIR_REJECT = 0x03;
  static constexpr uint8_t MODE_PLAYBACK = 0x01;
  static constexpr uint8_t MODE_SHOOT = 0x02;
  static constexpr uint8_t MODE_WAKE = 0x03;

  static constexpr uint8_t GEO_REQUEST = 0x03;
  const std::array<uint8_t, 1> GEO_ENABLE = {0x01};
  static constexpr uint8_t GEO_SUCCESS = 0x02;

  volatile uint8_t m_PairResult = 0x00;
  NimBLERemoteCharacteristic *m_Geo = nullptr;
  bool m_GeoEnabled = false;

  bool _connect(void) override final;

  void pairCallback(NimBLERemoteCharacteristic *, uint8_t *, size_t, bool);
};

}  // namespace Furble
#endif
