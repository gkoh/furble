#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "Device.h"
#include "Fujifilm.h"

namespace Furble {

constexpr std::array<uint8_t, 2> Fujifilm::SHUTTER_RELEASE;
constexpr std::array<uint8_t, 2> Fujifilm::SHUTTER_CMD;
constexpr std::array<uint8_t, 2> Fujifilm::SHUTTER_PRESS;
constexpr std::array<uint8_t, 2> Fujifilm::SHUTTER_FOCUS;

void Fujifilm::notify(BLERemoteCharacteristic *pChr, uint8_t *pData, size_t length, bool isNotify) {
  ESP_LOGI(LOG_TAG, "Got %s (%u bytes) from %s", isNotify ? "notification" : "indication", length,
           pChr->getUUID().toString().c_str());
  if (length > 0) {
    ESP_LOGI(LOG_TAG, " %s", NimBLEUtils::dataToHexString(pData, length).c_str());
  }

  if (pChr->getUUID() == CHR_NOT1_UUID) {
    if ((length >= 2) && (pData[0] == 0x02) && (pData[1] == 0x00)) {
      m_Configured = true;
    }
  } else if (pChr->getUUID() == GEOTAG_UPDATE) {
    if ((length >= 2) && (pData[0] == 0x01) && (pData[1] == 0x00)) {
      m_GeoRequested = true;
    }
  } else {
    ESP_LOGW(LOG_TAG, "Unhandled subscription.");
  }
}

bool Fujifilm::subscribe(const NimBLEUUID &svc, const NimBLEUUID &chr, bool notification) {
  auto pSvc = m_Client->getService(svc);
  if (pSvc == nullptr) {
    return false;
  }

  auto pChr = pSvc->getCharacteristic(chr);
  if (pChr == nullptr) {
    return false;
  }

  return pChr->subscribe(
      notification,
      [this](BLERemoteCharacteristic *pChr, uint8_t *pData, size_t length, bool isNotify) {
        this->notify(pChr, pData, length, isNotify);
      },
      true);
}

/**
 * Determine if the advertised BLE device is a Fujifilm.
 */
bool Fujifilm::matches(const NimBLEAdvertisedDevice *pDevice) {
  if (pDevice->haveManufacturerData()
      && pDevice->getManufacturerData().length() >= sizeof(fujifilm_adv_t)) {
    const fujifilm_adv_t adv = pDevice->getManufacturerData<fujifilm_adv_t>();
    return (adv.company_id == COMPANY_ID);
  }
  return false;
}

template <std::size_t N>
void Fujifilm::sendShutterCommand(const std::array<uint8_t, N> &cmd,
                                  const std::array<uint8_t, N> &param) {
  if (m_Shutter != nullptr && m_Shutter->canWrite()) {
    m_Shutter->writeValue(cmd.data(), sizeof(cmd), true);
    m_Shutter->writeValue(param.data(), sizeof(cmd), true);
  }
}

void Fujifilm::shutterPress(void) {
  sendShutterCommand(SHUTTER_CMD, SHUTTER_PRESS);
}

void Fujifilm::shutterRelease(void) {
  sendShutterCommand(SHUTTER_CMD, SHUTTER_RELEASE);
}

void Fujifilm::focusPress(void) {
  sendShutterCommand(SHUTTER_CMD, SHUTTER_FOCUS);
}

void Fujifilm::focusRelease(void) {
  shutterRelease();
}

void Fujifilm::sendGeoData(const gps_t &gps, const timesync_t &timesync) {
  NimBLERemoteService *pSvc = m_Client->getService(SVC_GEOTAG_UUID);
  if (pSvc == nullptr) {
    return;
  }

  NimBLERemoteCharacteristic *pChr = pSvc->getCharacteristic(CHR_GEOTAG_UUID);
  if (pChr == nullptr) {
    return;
  }

  if (pChr->canWrite()) {
    geotag_t geotag = {
        .latitude = (int32_t)(gps.latitude * 10000000),
        .longitude = (int32_t)(gps.longitude * 10000000),
        .altitude = (int32_t)gps.altitude,
        .pad = {0},
        .gps_time = {
                .year = (uint16_t)timesync.year,
                .day = (uint8_t)timesync.day,
                .month = (uint8_t)timesync.month,
                .hour = (uint8_t)timesync.hour,
                .minute = (uint8_t)timesync.minute,
                .second = (uint8_t)timesync.second,
                }
    };

    ESP_LOGI(LOG_TAG,
             "Sending geotag data (%u bytes) to 0x%04x\r\n"
             "  lat: %f, %d\r\n"
             "  lon: %f, %d\r\n"
             "  alt: %f, %d\r\n",
             sizeof(geotag), pChr->getHandle(), gps.latitude, geotag.latitude, gps.longitude,
             geotag.longitude, gps.altitude, geotag.altitude);

    pChr->writeValue((uint8_t *)&geotag, sizeof(geotag), true);
  }
}

void Fujifilm::updateGeoData(const gps_t &gps, const timesync_t &timesync) {
  if (m_GeoRequested) {
    sendGeoData(gps, timesync);
    m_GeoRequested = false;
  }
}

void Fujifilm::_disconnect(void) {
  m_Client->disconnect();
}

}  // namespace Furble
