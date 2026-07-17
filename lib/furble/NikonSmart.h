#ifndef NIKON_SMART_H
#define NIKON_SMART_H

#include <array>
#include <cstdint>
#include <vector>

#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>
#include <NimBLEUUID.h>

#include "Blowfish.h"
#include "Camera.h"
#include "NikonBase.h"

namespace Furble {

/**
 * Nikon smart-device (Blowfish-crypted handshake).
 *
 * Currently non-functional due to requirement on Bluetooth Classic to exchange
 * secrets.
 */
class NikonSmart: public NikonBase {
 public:
  NikonSmart(NimBLEClient *client,
             QueueHandle_t queue,
             NimBLERemoteCharacteristic *pairChr,
             const NikonBase::Pairing::id_t &id,
             const uint64_t timestamp,
             std::atomic<uint8_t> *progress);

  void shutterPress(void) override final;
  void shutterRelease(void) override final;
  void focusPress(void) override final;
  void focusRelease(void) override final;
  void updateGeoData(const Camera::gps_t &gps, const Camera::timesync_t &timesync) override final;

  /** Pair characteristic UUID. */
  static const NimBLEUUID PAIR_CHR_UUID;

 private:
  class SmartPairing: public NikonBase::Pairing, private Blowfish {
   public:
    SmartPairing(const uint64_t timestamp, const NikonBase::Pairing::id_t id);
    const msg_t *processMessage(const msg_t &msg) override final;
    std::array<uint32_t, 2> hash(const uint32_t *src, size_t len) const;

   private:
    void scramble(uint32_t *pL, uint32_t *pR) const;
    int8_t findSaltIndex(const msg_t &msg);

    static const std::vector<uint8_t> KEY;
    static const std::array<std::array<uint32_t, 2>, 8> SALT;
    int8_t m_Salt = -1;
  };

  // Smart characteristic UUIDs (instance).
  const NimBLEUUID NOT1_CHR_UUID {0x00002008, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID ID_CHR_UUID {0x00002002, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID GEO_CHR_UUID {0x00002007, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  // Carried forward (currently unused).
  const NimBLEUUID NOT2_CHR_UUID {0x0000200a, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID TIME_CHR_UUID {0x00002006, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID UNK0_CHR_UUID {0x00002009, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID UNK1_CHR_UUID {0x00002009, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID UNK2_CHR_UUID {0x00002080, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID UNK3_CHR_UUID {0x00002086, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID UNK4_CHR_UUID {0x00002001, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID UNK5_CHR_UUID {0x00002082, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID UNK6_CHR_UUID {0x00002084, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
  const NimBLEUUID UNK7_CHR_UUID {0x00002001, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};

  // Notification responses.
  static constexpr std::array<uint8_t, 2> SUCCESS = {0x01, 0x00};
  // Carried forward (currently unused).
  static constexpr std::array<uint8_t, 2> GEO = {0x00, 0x01};

  /** Time synchronisation. */
  typedef struct __attribute__((packed)) _nikon_time_t {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
  } nikon_time_t;

  // 10 bytes (carried forward; currently unused).
  typedef struct __attribute__((packed)) _timesync_msg_t {
    nikon_time_t time;
    uint8_t dst_offset;
    uint8_t tz_offset_hours;
    uint8_t tz_offset_minutes;
  } timesync_msg_t;

  // 41 bytes
  /** Location synchronisation. */
  typedef struct __attribute__((packed)) _nikon_geo_t {
    uint16_t header;             // 0x7f00 = isLat | isLon | isSat | isAlt |
                                 // isPos | isGps | isMap
    uint8_t latitude_direction;  // {N|S}
    uint8_t latitude_degrees;
    uint8_t latitude_minutes;
    uint8_t latitude_submin1;     // Remaining fractional minutes(hundredths of a minute)
    uint8_t latitude_submin2;     // Remaining fractional hundredths-of-a-minute
    uint8_t longitude_direction;  // {E|W}
    uint8_t longitude_degrees;
    uint8_t longitude_minutes;
    uint8_t longitude_submin1;  // Remaining fractional minutes(hundredths of a minute)
    uint8_t longitude_submin2;  // Remaining fractional hundredths-of-a-minute
    uint8_t satellites;         // no. of satellites
    uint8_t altitude_ref;       // P=0x50 for positive, M=0x4D for negative altitude
    uint16_t altitude;
    nikon_time_t time;
    uint8_t subseconds;
    uint8_t valid;        // 0x01 == valid
    uint8_t standard[6];  // WGS-84
    uint8_t pad[10];
  } nikon_geo_t;

  bool preSubscribe(NimBLERemoteService *pSvc) override final;
  bool connectFinalise(void) override final;

  void degreesToDMSubMin(double value,
                         uint8_t &degrees,
                         uint8_t &minutes,
                         uint8_t &submin1,
                         uint8_t &submin2);
};

}  // namespace Furble
#endif
