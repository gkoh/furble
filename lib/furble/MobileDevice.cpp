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

MobileDevice::MobileDevice(NimBLEAddress address) {
  m_Name = address.toString();
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
  float progress = 0.0f;
  updateProgress(pFunc, pCtx, progress);

  m_HIDServer->start(60, nullptr, &m_Address);

  unsigned int timeout = 60;

  Serial.println("Waiting for connection.");
  while (--timeout && !isConnected()) {
    progress += 1.0f;
    updateProgress(pFunc, pCtx, progress);
    delay(1000);
  };

  if (timeout == 0) {
    Serial.println("Connection timed out.");
    return false;
  }

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

void MobileDevice::updateGeoData(gps_t &gps, timesync_t &timesync) {
  // not supported
}

void MobileDevice::disconnect(void) {
  m_HIDServer->disconnect(m_Address);
}

bool MobileDevice::isConnected(void) {
  return m_HIDServer->isConnected();
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
