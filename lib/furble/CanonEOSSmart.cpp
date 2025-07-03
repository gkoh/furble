#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "CanonEOSSmart.h"

namespace Furble {

const NimBLEUUID CanonEOSSmart::PRI_SVC_UUID {0x00010000, 0x0000, 0x1000, 0x0000d8492fffa821};
constexpr uint8_t CanonEOSSmart::MODE_SHOOT;

bool CanonEOSSmart::matches(const NimBLEAdvertisedDevice *pDevice) {
  if (pDevice->haveServiceUUID()) {
    auto uuid = pDevice->getServiceUUID();

    return (uuid == PRI_SVC_UUID);
  }

  return false;
}

// Handle pairing notification
void CanonEOSSmart::pairCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                                 uint8_t *pData,
                                 size_t length,
                                 bool isNotify) {
  if (!isNotify && (length > 0)) {
    ESP_LOGI(LOG_TAG, "Got pairing callback: 0x%02x", pData[0]);
    m_PairResult = pData[0];
  }
}

/**
 * Connect to a Canon EOS in smart device mode.
 *
 * The EOS uses the 'just works' BLE bonding to pair, all bond management is
 * handled by the underlying NimBLE and ESP32 libraries.
 */
bool CanonEOSSmart::_connect(void) {
  if (NimBLEDevice::isBonded(m_Address)) {
    // Already bonded? Assume pair acceptance!
    m_PairResult = PAIR_ACCEPT;
  } else {
    m_PairResult = 0x00;
  }

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

  NimBLERemoteService *pSvc = m_Client->getService(PRI_SVC_UUID);
  if (pSvc) {
    NimBLERemoteCharacteristic *pChr = pSvc->getCharacteristic(CHR_NAME_UUID);
    if ((pChr != nullptr) && pChr->canIndicate()) {
      ESP_LOGI(LOG_TAG, "Subscribed for pairing indication");
      pChr->subscribe(false,
                      [this](BLERemoteCharacteristic *pChr, uint8_t *pData, size_t length,
                             bool isNotify) { this->pairCallback(pChr, pData, length, isNotify); });
    }
  }

  ESP_LOGI(LOG_TAG, "Identifying 1!");
  const auto name = Device::getStringID();
  if (!writePrefix(PRI_SVC_UUID, CHR_NAME_UUID, 0x01, name.c_str(), name.length()))
    return false;

  m_Progress = 30;

  ESP_LOGI(LOG_TAG, "Identifying 2!");
  if (!writePrefix(PRI_SVC_UUID, CHR_IDEN_UUID, 0x03, m_Uuid.uint8, Device::UUID128_LEN))
    return false;

  m_Progress = 40;

  ESP_LOGI(LOG_TAG, "Identifying 3!");
  if (!writePrefix(PRI_SVC_UUID, CHR_IDEN_UUID, 0x04, name.c_str(), name.length()))
    return false;

  m_Progress = 50;

  ESP_LOGI(LOG_TAG, "Identifying 4!");

  std::array<uint8_t, 1> x = {0x02};
  if (!writePrefix(PRI_SVC_UUID, CHR_IDEN_UUID, 0x05, x.data(), x.size())) {
    return false;
  }

  m_Progress = 60;

  ESP_LOGI(LOG_TAG, "Identifying 5!");

  // Give the user 60s to confirm/deny pairing
  ESP_LOGI(LOG_TAG, "Waiting for user to confirm/deny pairing.");
  for (unsigned int i = 0; i < 60; i++) {
    m_Progress = m_Progress.load() + (i % 2);
    if (m_PairResult != 0x00) {
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  if (m_PairResult != PAIR_ACCEPT) {
    bool deleted = NimBLEDevice::deleteBond(m_Address);
    ESP_LOGW(LOG_TAG, "Rejected, delete pairing: %d", deleted);
    return false;
  }

  ESP_LOGI(LOG_TAG, "Paired!");

  /* write to 0xf104 */
  x = {0x01};
  if (!m_Client->setValue(PRI_SVC_UUID, CHR_IDEN_UUID, {x.data(), x.size()}))
    return false;

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

  m_Progress = 90;

  ESP_LOGI(LOG_TAG, "Switching mode!");

  /* write to 0xf307 */
  if (!m_Client->setValue(SVC_MODE_UUID, CHR_MODE_UUID, {&MODE_SHOOT, sizeof(MODE_SHOOT)}))
    return false;

  ESP_LOGI(LOG_TAG, "Done!");
  m_Progress = 100;

  return true;
}

void CanonEOSSmart::shutterPress(void) {
  const std::array<uint8_t, 2> x = {0x00, 0x01};
  m_Client->setValue(SVC_SHUTTER_UUID, CHR_SHUTTER_UUID, {x.data(), x.size()});
}

void CanonEOSSmart::shutterRelease(void) {
  const std::array<uint8_t, 2> x = {0x00, 0x02};
  m_Client->setValue(SVC_SHUTTER_UUID, CHR_SHUTTER_UUID, {x.data(), x.size()});
}

void CanonEOSSmart::focusPress(void) {
  // do nothing
  return;
}

void CanonEOSSmart::focusRelease(void) {
  // do nothing
  return;
}

void CanonEOSSmart::updateGeoData(const gps_t &gps, const timesync_t &timesync) {
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
