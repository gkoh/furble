#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include <esp_random.h>

#include "Device.h"
#include "Nikon.h"

#define NIKON_DEBUG (1)

namespace Furble {

const NimBLEUUID Nikon::SERVICE_UUID {0x0000de00, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
const std::array<uint8_t, 2> Nikon::SUCCESS = {0x01, 0x00};
const std::array<uint8_t, 2> Nikon::GEO = {0x00, 0x01};

Nikon::Nikon(const void *data, size_t len) : Camera(Type::NIKON, PairType::SAVED) {
  if (len != sizeof(nikon_t))
    abort();

  const nikon_t *nikon = static_cast<const nikon_t *>(data);
  m_Name = std::string(nikon->name);
  m_Address = NimBLEAddress(nikon->address, nikon->type);
  memcpy(&m_ID, &nikon->id, sizeof(m_ID));
  m_RemotePair[0].id = m_ID;
  m_Queue = xQueueCreate(3, sizeof(bool));
}

Nikon::Nikon(const NimBLEAdvertisedDevice *pDevice) : Camera(Type::NIKON, PairType::NEW) {
  m_Name = pDevice->getName();
  m_Address = pDevice->getAddress();
  esp_fill_random(&m_ID, sizeof(m_ID));
  m_RemotePair[0].id = m_ID;
  m_Queue = xQueueCreate(3, sizeof(bool));
}

Nikon::~Nikon(void) {
  vQueueDelete(m_Queue);
}

/**
 * Determine if the advertised BLE device is a Nikon.
 */
bool Nikon::matches(const NimBLEAdvertisedDevice *pDevice) {
  if (pDevice->haveServiceUUID()) {
    auto uuid = pDevice->getServiceUUID();

    return (uuid == SERVICE_UUID);
  }

  return false;
}

void Nikon::onResult(const NimBLEAdvertisedDevice *pDevice) {
  if (Nikon::matches(pDevice)) {
    if (pDevice->haveManufacturerData()) {
      nikon_adv_t saved = {COMPANY_ID, m_ID.device, 0x00};
      nikon_adv_t found = pDevice->getManufacturerData<nikon_adv_t>();

      if (memcmp(&saved, &found, sizeof(saved)) == 0) {
        m_Address = pDevice->getAddress();
        bool success = true;
        xQueueSend(m_Queue, &success, 0);
      }
    }
  }
}

/**
 * Connect to a Nikon.
 */
bool Nikon::_connect(void) {
  bool success = false;
  m_Progress = 0;
  m_Task = xTaskGetCurrentTaskHandle();

  if (m_PairType == PairType::SAVED) {
    ESP_LOGI(LOG_TAG, "Scanning");
    // need to scan for advertising camera
    auto &scan = Scan::getInstance();
    scan.clear();
    scan.start(this, SCAN_TIME_MS);
    m_Progress += 10;

    // wait up to 60s for camera to appear
    BaseType_t timeout = xQueueReceive(m_Queue, &success, pdMS_TO_TICKS(60000));
    scan.stop();

    if (timeout == pdFALSE) {
      ESP_LOGI(LOG_TAG, "Timeout waiting for camera");
      return false;
    }
  }

  ESP_LOGI(LOG_TAG, "Connecting to %s", m_Address.toString().c_str());
  if (!m_Client->connect(m_Address)) {
    ESP_LOGI(LOG_TAG, "Connection failed!!!");
    return false;
  }

  ESP_LOGI(LOG_TAG, "Connected");
  m_Progress += 10;

  ESP_LOGI(LOG_TAG, "Getting service UUID");
  auto *pSvc = m_Client->getService(SERVICE_UUID);
  if (pSvc == nullptr) {
    return false;
  }

  ESP_LOGI(LOG_TAG, "Got service UUID!");
  m_Progress += 10;

  volatile bool failed = false;
  volatile uint8_t stage = 0x00;

  auto *pChr = pSvc->getCharacteristic(NOT1_CHR_UUID);
  if (pChr == nullptr) {
    return false;
  }

  if (!pChr->subscribe(
          true,
          [this](NimBLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData,
                 size_t length, bool isNotify) {
            bool rc = false;
#if NIKON_DEBUG
            ESP_LOGI(LOG_TAG, "data(not1) = %s",
                     NimBLEUtils::dataToHexString(pData, length).c_str());
#endif
            if (memcmp(pData, SUCCESS.data(), length) == 0) {
              rc = true;
            }
            xQueueSend(m_Queue, &rc, 0);
          },
          true)) {
    return false;
  }

  pChr = pSvc->getCharacteristic(IND2_CHR_UUID);
  if (pChr == nullptr) {
    return false;
  }
  if (!pChr->subscribe(
          false,
          [this, &stage, &failed](NimBLERemoteCharacteristic *pBLERemoteCharacteristic,
                                  uint8_t *pData, size_t length, bool isNotify) {
            bool rc = false;
#if NIKON_DEBUG
            ESP_LOGI(LOG_TAG, "data(ind2) = %s",
                     NimBLEUtils::dataToHexString(pData, length).c_str());
#endif
            if (memcmp(pData, &m_RemotePair[stage + 1], length) == 0) {
              rc = true;
            } else {
              ESP_LOGI(LOG_TAG, "Stage %u response mismatch", stage);
              rc = false;
            }
            xQueueSend(m_Queue, &rc, 0);
          },
          true)) {
    return false;
  }

  // perform four stage handshake
  for (; stage < 4 && !failed; stage += 2) {
    if (!m_Client->setValue(
            SERVICE_UUID, IND2_CHR_UUID,
            {(const uint8_t *)&m_RemotePair[stage], (uint16_t)sizeof(m_RemotePair[stage])}, true)) {
      return false;
    }
#if NIKON_DEBUG
    ESP_LOGI(LOG_TAG, "sent = %s",
             NimBLEUtils::dataToHexString((const uint8_t *)&m_RemotePair[stage],
                                          sizeof(m_RemotePair[stage]))
                 .c_str());
#endif

    // wait 10s for a notification
    BaseType_t timeout = xQueueReceive(m_Queue, &success, pdMS_TO_TICKS(10000));
    if (timeout == pdFALSE) {
      success = false;
    }
    m_Progress += 5;
  }

  if (!success) {
    return false;
  }

  const auto name = Device::getStringID();
  ESP_LOGI(LOG_TAG, "Identifying as %s", name.c_str());
  if (!m_Client->setValue(SERVICE_UUID, ID_CHR_UUID, name, true)) {
    return false;
  }

  m_Progress += 10;

  // wait for final OK
  BaseType_t timeout = xQueueReceive(m_Queue, &success, pdMS_TO_TICKS(10000));
  if (timeout == pdFALSE) {
    success = false;
  }

  ESP_LOGI(LOG_TAG, "%s", success ? "Done!" : "Failed to receive final OK.");

  m_Progress = 100;

  return success;
}

void Nikon::shutterPress(void) {
  std::array<uint8_t, 2> cmd = {MODE_SHUTTER, CMD_PRESS};
  m_Client->setValue(SERVICE_UUID, REMOTE_SHUTTER_CHR_UUID, {cmd.data(), cmd.size()}, false);
}

void Nikon::shutterRelease(void) {
  std::array<uint8_t, 2> cmd = {MODE_SHUTTER, CMD_RELEASE};
  m_Client->setValue(SERVICE_UUID, REMOTE_SHUTTER_CHR_UUID, {cmd.data(), cmd.size()}, false);
}

void Nikon::focusPress(void) {
  // do nothing
}

void Nikon::focusRelease(void) {
  // do nothing
}

void Nikon::updateGeoData(const gps_t &gps, const timesync_t &timesync) {
  nikon_time_t ntime = {
      .year = (uint16_t)timesync.year,
      .month = (uint8_t)timesync.month,
      .day = (uint8_t)timesync.day,
      .hour = (uint8_t)timesync.hour,
      .minute = (uint8_t)timesync.minute,
      .second = (uint8_t)timesync.second,
  };

  nikon_geo_t geo = {
      .header = 0x007f,
      .latitude_direction = gps.latitude < 0.0 ? 'S' : 'N',
      .latitude_degrees = 0,
      .latitude_minutes = 0,
      .latitude_seconds = 0,
      .latitude_fraction = 0,
      .longitude_direction = gps.longitude < 0.0 ? 'W' : 'E',
      .longitude_degrees = 0,
      .longitude_minutes = 0,
      .longitude_seconds = 0,
      .longitude_fraction = 0,
      .unknown0 = {0x00, 0x50},
      .altitude = (uint16_t)gps.altitude,
      .time = ntime,
      .unknown1 = {0x1e, 0x01},
      .standard = {'W', 'G', 'S', '-', '8', '4'},
      .pad = {0x00},
  };

  degreesToDMS(gps.latitude, geo.latitude_degrees, geo.latitude_minutes, geo.latitude_seconds,
               geo.latitude_fraction);
  degreesToDMS(gps.longitude, geo.longitude_degrees, geo.longitude_minutes, geo.longitude_seconds,
               geo.longitude_fraction);

  ESP_LOGI(LOG_TAG, "sending GPS = %s",
           NimBLEUtils::dataToHexString((const uint8_t *)&geo, sizeof(geo)).c_str());
  if (m_Client->setValue(SERVICE_UUID, GEO_CHR_UUID, {(const uint8_t *)&geo, (uint16_t)sizeof(geo)},
                         true)) {
    ESP_LOGI(LOG_TAG, "  success");
  } else {
    ESP_LOGI(LOG_TAG, "  failed");
  }
}

void Nikon::_disconnect(void) {
  m_Client->disconnect();
  m_Connected = false;
}

void Nikon::degreesToDMS(double value,
                         uint8_t &degrees,
                         uint8_t &minutes,
                         uint8_t &seconds,
                         uint8_t &fraction) {
  double integral;
  double fractional = std::modf(std::fabs(value), &integral);
  degrees = (uint8_t)integral;
  fractional *= 60;
  fractional = std::modf(fractional, &integral);
  minutes = (uint8_t)integral;
  fractional *= 60;
  fractional = std::modf(fractional, &integral);
  seconds = (uint8_t)integral;
  fractional *= 10;
  fractional = std::modf(fractional, &integral);
  fraction = (uint8_t)integral;
}

size_t Nikon::getSerialisedBytes(void) const {
  return sizeof(nikon_t);
}

bool Nikon::serialise(void *buffer, size_t bytes) const {
  if (bytes != sizeof(nikon_t)) {
    return false;
  }
  nikon_t *x = static_cast<nikon_t *>(buffer);
  strncpy(x->name, m_Name.c_str(), MAX_NAME);
  x->address = (uint64_t)m_Address;
  x->type = m_Address.getType();
  memcpy(&x->id, &m_ID, sizeof(x->id));

  return true;
}

}  // namespace Furble
