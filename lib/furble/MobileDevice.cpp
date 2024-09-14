#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "Device.h"
#include "MobileDevice.h"

namespace Furble {

MobileDevice::MobileDevice(const void *data, size_t len) {
  if (len != sizeof(mobile_device_t))
    throw;

  const mobile_device_t *mobile_device = static_cast<const mobile_device_t *>(data);
  m_Name = std::string(mobile_device->name);
  m_Address = NimBLEAddress(mobile_device->address, mobile_device->type);
  m_HIDServer = HIDServer::getInstance();
}

MobileDevice::MobileDevice(const NimBLEAddress &address, const std::string &name) {
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
bool MobileDevice::connect(progressFunc pFunc, void *pCtx) {
  unsigned int timeout_secs = 60;
  float progress = 0.0f;
  updateProgress(pFunc, pCtx, progress);

  m_HIDServer->start(&m_Address);

  ESP_LOGI(LOG_TAG, "Waiting for %us for connection from %s", timeout_secs, m_Name.c_str());
  while (--timeout_secs && !isConnected()) {
    progress += 1.0f;
    updateProgress(pFunc, pCtx, progress);
    delay(1000);
  };

  if (timeout_secs == 0) {
    ESP_LOGI(LOG_TAG, "Connection timed out.");
    return false;
  }

  ESP_LOGI(LOG_TAG, "Connected to %s.", m_Name.c_str());
  progress = 100.0f;
  updateProgress(pFunc, pCtx, progress);
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

void MobileDevice::disconnect(void) {
  m_HIDServer->disconnect(m_Address);
}

bool MobileDevice::isConnected(void) {
  return m_HIDServer->isConnected(m_Address);
}

device_type_t MobileDevice::getDeviceType(void) {
  return FURBLE_MOBILE_DEVICE;
}

size_t MobileDevice::getSerialisedBytes(void) {
  return sizeof(mobile_device_t);
}

bool MobileDevice::serialise(void *buffer, size_t bytes) {
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
