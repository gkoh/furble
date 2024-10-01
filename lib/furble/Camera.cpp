#include <NimBLEAdvertisedDevice.h>

#include "Camera.h"

namespace Furble {

Camera::Camera(Type type) : m_Type(type) {
  m_Client = NimBLEDevice::createClient();
}

Camera::~Camera() {
  NimBLEDevice::deleteClient(m_Client);
  m_Client = nullptr;
}

bool Camera::connect(esp_power_level_t power) {
  const std::lock_guard<std::mutex> lock(m_Mutex);

  // try extending range by adjusting connection parameters
  bool connected = this->_connect();
  if (connected) {
    if (m_Type != Type::FAUXNY) {
      // Set BLE transmit power after connection is established.
      NimBLEDevice::setPower(power);
      m_Client->updateConnParams(m_MinInterval, m_MaxInterval, m_Latency, m_Timeout);
    }
    m_Connected = true;
  } else {
    m_Connected = false;
  }

  return m_Connected;
}

void Camera::disconnect(void) {
  const std::lock_guard<std::mutex> lock(m_Mutex);
  m_Active = false;
  m_Progress = 0;
  this->_disconnect();
}

bool Camera::isActive(void) const {
  return m_Active;
}

void Camera::setActive(bool active) {
  m_Active = active;
}

const Camera::Type &Camera::getType(void) const {
  return m_Type;
}

const std::string &Camera::getName(void) const {
  return m_Name;
}

const NimBLEAddress &Camera::getAddress(void) const {
  return m_Address;
}

uint8_t Camera::getConnectProgress(void) const {
  return m_Progress.load();
}

bool Camera::isConnected(void) const {
  if (m_Type == Type::FAUXNY) {
    return m_Connected;
  }

  return m_Connected && m_Client->isConnected();
}

}  // namespace Furble
