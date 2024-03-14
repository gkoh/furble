#include <NimBLEAdvertisedDevice.h>

#include "Camera.h"

namespace Furble {

bool Camera::connect(esp_power_level_t power, progressFunc pFunc, void *pCtx) {
  bool connected = this->connect(pFunc, pCtx);
  if (connected) {
    // Set BLE transmit power after connection is established.
    NimBLEDevice::setPower(power);
  }

  return connected;
}

const char *Camera::getName(void) {
  return m_Name.c_str();
}

void Camera::fillSaveName(char *name) {
  snprintf(name, 16, "%08llX", (uint64_t)m_Address);
}

void Camera::updateProgress(progressFunc pFunc, void *ctx, float value) {
  if (pFunc != nullptr) {
    (pFunc)(ctx, value);
  }
}

bool Camera::isConnected(void) {
  return m_Client->isConnected();
}

}  // namespace Furble
