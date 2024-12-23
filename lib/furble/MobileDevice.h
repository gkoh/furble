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
  MobileDevice(const NimBLEAddress &address, const std::string &name);
  ~MobileDevice(void);

  static bool matches(NimBLEAdvertisedDevice *pDevice);

  void shutterPress(void) override;
  void shutterRelease(void) override;
  void focusPress(void) override;
  void focusRelease(void) override;
  void updateGeoData(const gps_t &gps, const timesync_t &timesync) override;

  bool isConnected(void) const override;

 protected:
  bool _connect(void) override;
  void _disconnect(void) override;

 private:
  typedef struct _mobile_device_t {
    char name[MAX_NAME]; /** Human readable device name. */
    uint64_t address;    /** BLE address. */
    uint8_t type;        /** Address type. */
  } mobile_device_t;

  size_t getSerialisedBytes(void) const override;
  bool serialise(void *buffer, size_t bytes) const override;
  void sendKeyReport(const uint8_t key);

  volatile bool m_DisconnectRequested = false;
  HIDServer *m_HIDServer;
};

}  // namespace Furble
#endif
