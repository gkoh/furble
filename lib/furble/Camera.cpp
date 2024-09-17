#include <NimBLEAdvertisedDevice.h>

#include "Camera.h"

namespace Furble {

Camera::Camera() {
  m_Client = NimBLEDevice::createClient();
}

Camera::~Camera() {
  NimBLEDevice::deleteClient(m_Client);
}

bool Camera::connect(esp_power_level_t power, progressFunc pFunc, void *pCtx) {
  // try extending range by adjusting connection parameters
  bool connected = this->connect(pFunc, pCtx);
  if (connected) {
    // Set BLE transmit power after connection is established.
    NimBLEDevice::setPower(power);
    m_Client->updateConnParams(m_MinInterval, m_MaxInterval, m_Latency, m_Timeout);
  }

  return connected;
}

bool Camera::isActive(void) {
  return m_Active;
}

void Camera::setActive(bool active) {
  m_Active = active;
}

const std::string &Camera::getName(void) {
  return m_Name;
}

const NimBLEAddress &Camera::getAddress(void) {
  return m_Address;
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
