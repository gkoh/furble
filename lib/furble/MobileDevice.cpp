#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>
#include <services/gap/ble_svc_gap.h>

#include "CameraList.h"
#include "Device.h"
#include "MobileDevice.h"

namespace Furble {

MobileDevice::MobileDevice(const void *data, size_t len)
    : Camera(Type::MOBILE_DEVICE, PairType::SAVED) {
  if (len != sizeof(mobile_device_t))
    abort();

  const mobile_device_t *mobile_device = static_cast<const mobile_device_t *>(data);
  m_Name = std::string(mobile_device->name);
  m_Address = NimBLEAddress(mobile_device->address, mobile_device->type);
  m_HIDServer = HIDServer::getInstance();
}

MobileDevice::MobileDevice(const NimBLEAddress &address, const std::string &name)
    : Camera(Type::MOBILE_DEVICE, PairType::NEW) {
  m_Name = name;
  m_Address = address;
  m_HIDServer = HIDServer::getInstance();
}

MobileDevice::~MobileDevice(void) {}

/**
 * Determine if the advertised BLE device is a mobile device.
 */
bool MobileDevice::matches(NimBLEAdvertisedDevice *pDevice) {
  return false;
}

/**
 * Connect to a mobile device.
 *
 * Mobile devices reverse the connection architecture such that furble is the server waiting for
 * incoming connections. We rely on the BLE 'just works' pairing and bonding to establish the
 * connection.
 * All this logic is encapsulated in the HIDServer class.
 */
bool MobileDevice::_connect(void) {
  unsigned int timeout_secs = 60;

  m_Progress = 0;
  m_DisconnectRequested = false;

  m_HIDServer->start(&m_Address);

  ESP_LOGI(LOG_TAG, "Waiting for %us for connection from %s", timeout_secs, m_Name.c_str());
  while (--timeout_secs && !isConnected()) {
    m_Progress = m_Progress.load() + 1;
    if (m_DisconnectRequested) {
      ESP_LOGI(LOG_TAG, "Disconnect requested.");
      return false;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  };

  if (timeout_secs == 0) {
    ESP_LOGI(LOG_TAG, "Connection timed out.");
    return false;
  }

  // Retrieve device name and update if needed
  auto server = NimBLEDevice::getServer();
  auto peerInfo = server->getPeerInfo(m_Address);
  auto peer = server->getClient(peerInfo);
  auto name = peer->getValue(NimBLEUUID((uint16_t)BLE_SVC_GAP_UUID16),
                             NimBLEUUID((uint16_t)BLE_SVC_GAP_CHR_UUID16_DEVICE_NAME));
  if (m_Name != name.c_str()) {
    m_Name = name.c_str();
    CameraList::save(this);
  }

  ESP_LOGI(LOG_TAG, "Connected to %s.", m_Name.c_str());
  m_Progress = 100;
  m_HIDServer->stop();

  return true;
}

void MobileDevice::sendKeyReport(const uint8_t key) {
  m_HIDServer->getInput()->setValue(&key, sizeof(key));
  m_HIDServer->getInput()->notify();
}

void MobileDevice::shutterPress(void) {
  sendKeyReport(0x01);
}

void MobileDevice::shutterRelease(void) {
  sendKeyReport(0x00);
}

void MobileDevice::focusPress(void) {
  // not supported
}

void MobileDevice::focusRelease(void) {
  // not supported
}

void MobileDevice::updateGeoData(const gps_t &gps, const timesync_t &timesync) {
  // not supported
}

void MobileDevice::_disconnect(void) {
  m_DisconnectRequested = true;
  m_HIDServer->disconnect(m_Address);
  m_Connected = false;
}

bool MobileDevice::isConnected(void) const {
  return m_HIDServer->isConnected(m_Address);
}

size_t MobileDevice::getSerialisedBytes(void) const {
  return sizeof(mobile_device_t);
}

bool MobileDevice::serialise(void *buffer, size_t bytes) const {
  if (bytes != sizeof(mobile_device_t)) {
    return false;
  }

  mobile_device_t *x = static_cast<mobile_device_t *>(buffer);
  strncpy(x->name, m_Name.c_str(), MAX_NAME);
  x->address = (uint64_t)m_Address;
  x->type = m_Address.getType();

  return true;
}

}  // namespace Furble
