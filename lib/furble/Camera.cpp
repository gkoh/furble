#include <NimBLEAdvertisedDevice.h>

#include "Camera.h"

namespace Furble {

Camera::Camera(Type type, PairType pairType) : m_PairType(pairType), m_Type(type) {}

Camera::~Camera() {
  m_Connected = false;
  m_Client = nullptr;
}

void Camera::onConnect(NimBLEClient *pClient) {
  // Set BLE transmit power after connection is established.
  NimBLEDevice::setPower(m_Power);
  m_Connected = true;
}

void Camera::onDisconnect(NimBLEClient *pClient, int reason) {
  m_Connected = false;
  m_Progress = 0;
}

bool Camera::connect(esp_power_level_t power, uint32_t timeout) {
  const std::lock_guard<std::mutex> lock(m_Mutex);

  m_Power = power;

  m_Client = NimBLEDevice::createClient();
  if (m_Client == nullptr) {
    ESP_LOGI(LOG_TAG, "Failed to create client");
    return false;
  }

  m_Client->setClientCallbacks(this, false);
  m_Client->setSelfDelete(true, true);  // self-delete on any connection failure

  // adjust connection timeout and parameters
  m_Client->setConnectTimeout(timeout);
  // try extending range by adjusting connection parameters
  m_Client->setConnectionParams(m_MinInterval, m_MaxInterval, m_Latency, m_Timeout);

  bool connected = this->_connect();
  if (connected) {
    m_Paired = true;
  } else {
    this->_disconnect();
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
  const std::lock_guard<std::mutex> lock(m_Mutex);
  if (m_Type == Type::FAUXNY) {
    return m_Connected;
  }

  return m_Connected && m_Client && m_Client->isConnected();
}

}  // namespace Furble
