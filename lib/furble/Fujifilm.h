#ifndef FUJIFILM_H
#define FUJIFILM_H

#include <NimBLERemoteCharacteristic.h>

#include "Camera.h"

namespace Furble {
/**
 * Fujifilm X.
 */
class Fujifilm: public Camera {
 public:
  Fujifilm(Type type, const void *data, size_t len) : Camera(type, PairType::SAVED) {};
  Fujifilm(Type type, const NimBLEAdvertisedDevice *pDevice) : Camera(type, PairType::NEW) {};

  /** Is the advertised device a Fujifilm device? */
  static bool matches(const NimBLEAdvertisedDevice *pDevice);

  void shutterPress(void) override final;
  void shutterRelease(void) override final;
  void focusPress(void) override final;
  void focusRelease(void) override final;
  void updateGeoData(const gps_t &gps, const timesync_t &timesync) override final;

 protected:
  /**
   * Advertisement manufacturer data.
   */
  typedef struct __attribute__((packed)) {
    uint16_t company_id;
    uint8_t type;
  } fujifilm_adv_t;

  static constexpr uint16_t COMPANY_ID = 0x04d8;

  // 0x4001
  const NimBLEUUID SVC_PAIR_UUID {0x91f1de68, 0xdff6, 0x466e, 0x8b65ff13b0f16fb8};
  // 0x4042
  const NimBLEUUID CHR_PAIR_UUID {0xaba356eb, 0x9633, 0x4e60, 0xb73ff52516dbd671};
  // 0x4012
  const NimBLEUUID CHR_IDEN_UUID {0x85b9163e, 0x62d1, 0x49ff, 0xa6f5054b4630d4a1};

  const NimBLEUUID SVC_CONF_UUID {0x4c0020fe, 0xf3b6, 0x40de, 0xacc977d129067b14};
  // 0x5013
  const NimBLEUUID CHR_IND1_UUID {0xa68e3f66, 0x0fcc, 0x4395, 0x8d4caa980b5877fa};
  // 0x5023
  const NimBLEUUID CHR_IND2_UUID {0xbd17ba04, 0xb76b, 0x4892, 0xa545b73ba1f74dae};
  // 0x5033
  const NimBLEUUID CHR_NOT1_UUID {0xf9150137, 0x5d40, 0x4801, 0xa8dcf7fc5b01da50};
  // 0x5043
  const NimBLEUUID CHR_NOT2_UUID {0xad06c7b7, 0xf41a, 0x46f4, 0xa29a712055319122};
  const NimBLEUUID CHR_IND3_UUID {0x049ec406, 0xef75, 0x4205, 0xa39008fe209c51f0};

  // Shutter characteristic
  const NimBLEUUID CHR_SHUTTER_UUID {0x7fcf49c6, 0x4ff0, 0x4777, 0xa03d1a79166af7a8};

  void _disconnect(void) override final;
  bool subscribe(const NimBLEUUID &svc, const NimBLEUUID &chr, bool notifications);

  bool m_Configured = false;
  NimBLERemoteCharacteristic *m_Shutter = nullptr;

 private:
  static constexpr uint8_t TYPE_TOKEN = 0x02;

  /**
   * Time synchronisation.
   */
  typedef struct __attribute__((packed)) _fujifilm_time_t {
    uint16_t year;
    uint8_t day;
    uint8_t month;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
  } fujifilm_time_t;

  /**
   * Location and time packet.
   */
  typedef struct __attribute__((packed)) _fujigeotag_t {
    int32_t latitude;
    int32_t longitude;
    int32_t altitude;
    uint8_t pad[4];
    fujifilm_time_t gps_time;
  } geotag_t;

  // Currently unused
  // const NimBLEUUID SVC_READ_UUID{0x4e941240, 0xd01d, 0x46b9, 0xa5ea67636806830b};
  // const NimBLEUUID CHR_READ_UUID{0xbf6dc9cf, 0x3606, 0x4ec9, 0xa4c8d77576e93ea4};

  const NimBLEUUID SVC_GEOTAG_UUID {0x3b46ec2b, 0x48ba, 0x41fd, 0xb1b8ed860b60d22b};
  const NimBLEUUID CHR_GEOTAG_UUID {0x0f36ec14, 0x29e5, 0x411a, 0xa1b664ee8383f090};

  const NimBLEUUID GEOTAG_UPDATE {0xad06c7b7, 0xf41a, 0x46f4, 0xa29a712055319122};

  static constexpr std::array<uint8_t, 2> SHUTTER_RELEASE = {0x00, 0x00};
  static constexpr std::array<uint8_t, 2> SHUTTER_CMD = {0x01, 0x00};
  static constexpr std::array<uint8_t, 2> SHUTTER_PRESS = {0x02, 0x00};
  static constexpr std::array<uint8_t, 2> SHUTTER_FOCUS = {0x03, 0x00};

  void notify(NimBLERemoteCharacteristic *, uint8_t *, size_t, bool);
  void sendGeoData(const gps_t &gps, const timesync_t &timesync);

  template <std::size_t N>
  void sendShutterCommand(const std::array<uint8_t, N> &cmd, const std::array<uint8_t, N> &param);

  volatile bool m_GeoRequested = false;
};

}  // namespace Furble
#endif
