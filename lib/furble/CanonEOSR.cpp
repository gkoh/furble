#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "CanonEOSR.h"

namespace Furble {

const NimBLEUUID CanonEOSR::PRI_SVC_UUID {0x00050000, 0x0000, 0x1000, 0x0000d8492fffa821};

bool CanonEOSR::matches(const NimBLEAdvertisedDevice *pDevice) {
  if (pDevice->haveServiceUUID()) {
    auto uuid = pDevice->getServiceUUID();

    return (uuid == PRI_SVC_UUID);
  }

  return false;
}

bool CanonEOSR::_connect(void) {
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

  pControl = pSvc->getCharacteristic(CTRL_CHR_UUID);
  if (!pControl) {
    return false;
  }
  ESP_LOGI(LOG_TAG, "Retrieved control service!");

  m_Progress = 100;
  ESP_LOGI(LOG_TAG, "Done!");

  return true;
}

void CanonEOSR::shutterPress(void) {
  const std::array<uint8_t, 1> cmd = {SHUTTER | CTRL};
  pControl->writeValue(cmd.data(), cmd.size(), true);
  return;
}

void CanonEOSR::shutterRelease(void) {
  const std::array<uint8_t, 1> cmd = {CTRL};
  pControl->writeValue(cmd.data(), cmd.size(), true);
  return;
}

void CanonEOSR::focusPress(void) {
  const std::array<uint8_t, 1> cmd = {FOCUS | CTRL};
  pControl->writeValue(cmd.data(), cmd.size(), true);
  return;
}

void CanonEOSR::focusRelease(void) {
  return shutterRelease();
}

void CanonEOSR::updateGeoData(const gps_t &gps, const timesync_t &timesync) {
  // do nothing
  return;
}

}  // namespace Furble
