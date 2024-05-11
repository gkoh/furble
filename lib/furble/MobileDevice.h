#ifndef MOBILEDEVICE_H
#define MOBILEDEVICE_H

#include "Camera.h"
#include "HIDServer.h"

namespace Furble {
/**
 * Mobile phones and tablets.
 *
 * Supports the camera application on both iOS and Android devices.
 */
class MobileDevice: public Camera {
 public:
  MobileDevice(const void *data, size_t len);
  MobileDevice(NimBLEAddress address);
  ~MobileDevice(void);

  static bool matches(NimBLEAdvertisedDevice *pDevice);

  bool connect(progressFunc pFunc = nullptr, void *pCtx = nullptr);
  void shutterPress(void);
  void shutterRelease(void);
  void focusPress(void);
  void focusRelease(void);
  void updateGeoData(gps_t &gps, timesync_t &timesync);
  void disconnect(void);

  bool isConnected(void);

 private:
  typedef struct _mobile_device_t {
    char name[MAX_NAME]; /** Human readable device name. */
    uint64_t address;    /** BLE address. */
    uint8_t type;        /** Address type. */
  } mobile_device_t;

  device_type_t getDeviceType(void);
  size_t getSerialisedBytes(void);
  bool serialise(void *buffer, size_t bytes);
  void sendKeyReport(const uint8_t key);

  HIDServer *m_HIDServer;
};

}  // namespace Furble
#endif
