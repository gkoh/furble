#ifndef FUJIFILM_H
#define FUJIFILM_H

#include "Device.h"

#define FUJIFILM_TOKEN_LEN (4)

namespace Furble {
/**
 * Fujifilm X.
 */
class Fujifilm: public Device {
 public:
  Fujifilm(const void *data, size_t len);
  Fujifilm(NimBLEAdvertisedDevice *pDevice);
  ~Fujifilm(void);

  /**
   * Determine if the advertised BLE device is a Fujifilm X-T30.
   */
  static bool matches(NimBLEAdvertisedDevice *pDevice);

  bool connect(ezProgressBar &progress_bar);
  void shutterPress(void);
  void shutterRelease(void);
  void focusPress(void);
  void focusRelease(void);
  void disconnect(void);
  void print(void);

 private:
  Device::type_t getDeviceType(void);
  size_t getSerialisedBytes(void);
  bool serialise(void *buffer, size_t bytes);

  uint8_t m_Token[FUJIFILM_TOKEN_LEN] = {0};
};

}  // namespace Furble
#endif
