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

  m_Control = pSvc->getCharacteristic(CTRL_CHR_UUID);
  if (!m_Control) {
    return false;
  }
  ESP_LOGI(LOG_TAG, "Retrieved control service!");

  ESP_LOGI(LOG_TAG, "Retrieving location service");
  pSvc = m_Client->getService(GEO_SVC_UUID);
  if (pSvc != nullptr) {
    m_Geo = pSvc->getCharacteristic(GEO_CHR_UUID);

    auto *pInd = pSvc->getCharacteristic(GEO_IND_UUID);
    if (pInd != nullptr) {
      m_Progress += 10;
      ESP_LOGI(LOG_TAG, "Subscribing to location service");
      pInd->subscribe(false, [this](BLERemoteCharacteristic *pChr, uint8_t *pData, size_t length,
                                    bool isNotify) {
        if (length > 0) {
          switch (pData[0]) {
            case GEO_REQUEST:
              if ((m_Geo != nullptr) && m_Geo->canWrite()) {
                m_Geo->writeValue(GEO_ENABLE.data(), GEO_ENABLE.size(), true);
              }
              break;
            case GEO_SUCCESS:
              m_GeoEnabled = true;
              break;
          }
        }
      });
      m_Progress += 10;
      ESP_LOGI(LOG_TAG, "Subscribed to location service!");
    }
    m_Progress += 10;
    ESP_LOGI(LOG_TAG, "Retrieved location service!");
  }

  m_Progress = 100;
  ESP_LOGI(LOG_TAG, "Done!");

  return true;
}

void CanonEOSR::shutterPress(void) {
  const std::array<uint8_t, 1> cmd = {SHUTTER | CTRL};
  m_Control->writeValue(cmd.data(), cmd.size(), true);
  return;
}

void CanonEOSR::shutterRelease(void) {
  const std::array<uint8_t, 1> cmd = {CTRL};
  m_Control->writeValue(cmd.data(), cmd.size(), true);
  return;
}

void CanonEOSR::focusPress(void) {
  const std::array<uint8_t, 1> cmd = {FOCUS | CTRL};
  m_Control->writeValue(cmd.data(), cmd.size(), true);
  return;
}

void CanonEOSR::focusRelease(void) {
  return shutterRelease();
}

void CanonEOSR::updateGeoData(const gps_t &gps, const timesync_t &timesync) {
  if (m_GeoEnabled) {
    struct tm time_str = {
        .tm_sec = static_cast<int>(timesync.second),
        .tm_min = static_cast<int>(timesync.minute),
        .tm_hour = static_cast<int>(timesync.hour),
        .tm_mday = static_cast<int>(timesync.day),
        .tm_mon = static_cast<int>(timesync.month - 1),
        .tm_year = static_cast<int>(timesync.year - 1900),
        .tm_wday = 0,
        .tm_yday = 0,
        .tm_isdst = -1,
    };

    time_t timestamp = mktime(&time_str);
    canon_geo_t geo = {
        .header = 0x04,
        .latitude_direction = gps.latitude < 0.0 ? 'S' : 'N',
        .latitude = (float)gps.latitude,
        .longitude_direction = gps.longitude < 0.0 ? 'W' : 'E',
        .longitude = (float)gps.longitude,
        .elevation_sign = gps.altitude < 0.0 ? '-' : '+',
        .elevation = (float)gps.altitude,
        .timestamp = static_cast<uint32_t>(timestamp),
    };
    if ((m_Geo != nullptr) && m_Geo->canWrite()) {
      m_Geo->writeValue(reinterpret_cast<const uint8_t *>(&geo), sizeof(geo), true);
    }
  }
  return;
}

}  // namespace Furble
