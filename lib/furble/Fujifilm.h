#ifndef FUJIFILM_H
#define FUJIFILM_H

#include <NimBLERemoteCharacteristic.h>

#include "Camera.h"

#define FUJIFILM_TOKEN_LEN (4)

namespace Furble {
/**
 * Fujifilm X.
 */
class Fujifilm: public Camera {
 public:
  Fujifilm(const void *data, size_t len);
  Fujifilm(const NimBLEAdvertisedDevice *pDevice);
  ~Fujifilm(void);

  /**
   * Determine if the advertised BLE device is a Fujifilm X-T30.
   */
  static bool matches(const NimBLEAdvertisedDevice *pDevice);

  bool connect(void) override;
  void shutterPress(void) override;
  void shutterRelease(void) override;
  void focusPress(void) override;
  void focusRelease(void) override;
  void updateGeoData(const gps_t &gps, const timesync_t &timesync) override;
  void disconnect(void) override;
  size_t getSerialisedBytes(void) const override;
  bool serialise(void *buffer, size_t bytes) const override;

 private:
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

  void print(void);
  void notify(NimBLERemoteCharacteristic *, uint8_t *, size_t, bool);
  bool subscribe(const NimBLEUUID &svc, const NimBLEUUID &chr, bool notifications);
  void sendGeoData(const gps_t &gps, const timesync_t &timesync);

  template <std::size_t N>
  void sendShutterCommand(const std::array<uint8_t, N> &cmd, const std::array<uint8_t, N> &param);

  std::array<uint8_t, FUJIFILM_TOKEN_LEN> m_Token = {0};

  bool m_Configured = false;

  volatile bool m_GeoRequested = false;
};

}  // namespace Furble
#endif
