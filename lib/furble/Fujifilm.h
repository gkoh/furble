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
  Fujifilm(NimBLEAdvertisedDevice *pDevice);
  ~Fujifilm(void);

  /**
   * Determine if the advertised BLE device is a Fujifilm X-T30.
   */
  static bool matches(NimBLEAdvertisedDevice *pDevice);

  bool connect(progressFunc pFunc = nullptr, void *pCtx = nullptr);
  void shutterPress(void);
  void shutterRelease(void);
  void focusPress(void);
  void focusRelease(void);
  void updateGeoData(gps_t &gps, timesync_t &timesync);
  void disconnect(void);
  void print(void);

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

  device_type_t getDeviceType(void);
  size_t getSerialisedBytes(void);
  bool serialise(void *buffer, size_t bytes);
  void notify(NimBLERemoteCharacteristic *, uint8_t *, size_t, bool);
  void sendGeoData(gps_t &gps, timesync_t &timesync);

  uint8_t m_Token[FUJIFILM_TOKEN_LEN] = {0};

  bool m_Configured = false;

  volatile bool m_GeoRequested = false;
};

}  // namespace Furble
#endif
