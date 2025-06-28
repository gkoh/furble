#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "CanonEOSRemote.h"

namespace Furble {

const NimBLEUUID CanonEOSRemote::PRI_SVC_UUID {0x00050000, 0x0000, 0x1000, 0x0000d8492fffa821};

bool CanonEOSRemote::matches(const NimBLEAdvertisedDevice *pDevice) {
  if (pDevice->haveServiceUUID()) {
    auto uuid = pDevice->getServiceUUID();

    return (uuid == PRI_SVC_UUID);
  }

  return false;
}

bool CanonEOSRemote::_connect(void) {
  ESP_LOGI(LOG_TAG, "Connecting");
  if (!m_Client->connect(m_Address)) {
    ESP_LOGI(LOG_TAG, "Connection failed!!!");
    return false;
  }

  ESP_LOGI(LOG_TAG, "Connected");
  m_Progress = 10;

  ESP_LOGI(LOG_TAG, "Securing");
  if (!m_Client->secureConnection()) {
    return false;
  }
  ESP_LOGI(LOG_TAG, "Secured!");
  m_Progress = 20;

  // send device name
  ESP_LOGI(LOG_TAG, "Identifying");
  const auto name = Device::getStringID();
  if (!writePrefix(PRI_SVC_UUID, ID_CHR_UUID, 0x03, name.c_str(), name.length())) {
    return false;
  }
  ESP_LOGI(LOG_TAG, "Identified!");
  m_Progress = 30;

  ESP_LOGI(LOG_TAG, "Retrieving control service");
  auto *pSvc = m_Client->getService(PRI_SVC_UUID);
  if (!pSvc) {
    return false;
  }

  m_Control = pSvc->getCharacteristic(CTRL_CHR_UUID);
  if (!m_Control) {
    return false;
  }
  ESP_LOGI(LOG_TAG, "Retrieved control service!");

  m_Progress = 100;
  ESP_LOGI(LOG_TAG, "Done!");

  return true;
}

void CanonEOSRemote::shutterPress(void) {
  const std::array<uint8_t, 1> cmd = {SHUTTER | CTRL};
  m_Control->writeValue(cmd.data(), cmd.size(), true);
  return;
}

void CanonEOSRemote::shutterRelease(void) {
  const std::array<uint8_t, 1> cmd = {CTRL};
  m_Control->writeValue(cmd.data(), cmd.size(), true);
  return;
}

void CanonEOSRemote::focusPress(void) {
  const std::array<uint8_t, 1> cmd = {FOCUS | CTRL};
  m_Control->writeValue(cmd.data(), cmd.size(), true);
  return;
}

void CanonEOSRemote::focusRelease(void) {
  return shutterRelease();
}

void CanonEOSRemote::updateGeoData(const gps_t &gps, const timesync_t &timesync) {
  // not supported in remote mode
  return;
}

}  // namespace Furble
