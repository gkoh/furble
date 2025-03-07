#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include <esp_random.h>

#include "Device.h"
#include "Nikon.h"
#include "blowfish.h"

#define NIKON_DEBUG (1)

namespace Furble {

const std::array<uint8_t, 8> Nikon::BLOWFISH_KEY = {0xff, 0xff, 0xaa, 0x55, 0x11, 0x22, 0x33, 0x00};
const std::array<std::array<uint32_t, 2>, 8> Nikon::BLOWFISH_SALT = {
    {
     {0x704066e4u, 0x0433d552u},
     {0xed4b8facu, 0x15f7e47bu},
     {0x24471f11u, 0x8b5ea1fcu},
     {0x05960c31u, 0x2b8c7f41u},
     {0xfda588c1u, 0xeba8b1f3u},
     {0x99166056u, 0x1bd3d550u},
     {0xcd32687fu, 0xa9e28a30u},
     {0x2a8fe834u, 0xdec7ebf4u},

     }
};
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
  bf_init(static_cast<const uint8_t *>(BLOWFISH_KEY.data()), BLOWFISH_KEY.size());  // @todo
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

  ESP_LOGI(LOG_TAG, "Securing");
  if (!m_Client->secureConnection()) {
    return false;
  }
  ESP_LOGI(LOG_TAG, "Secured!");
  m_Progress += 10;

  auto *pSvc = m_Client->getService(SERVICE_UUID);
  if (pSvc == nullptr) {
    return false;
  }

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

  // try as remote connection first
  m_PairChr = pSvc->getCharacteristic(REMOTE_PAIR_CHR_UUID);
  if (m_PairChr == nullptr) {
    // then try as smart device
    m_PairChr = pSvc->getCharacteristic(PAIR_CHR_UUID);
    if (m_PairChr == nullptr) {
      return false;
    }
    ESP_LOGI(LOG_TAG, "Connecting as smart device");
  } else {
    ESP_LOGI(LOG_TAG, "Connecting as remote");
  }

  if (!m_PairChr->subscribe(
          false,
          [this](NimBLERemoteCharacteristic *pBLERemoteCharacteristic,
                                  uint8_t *pData, size_t length, bool isNotify) {
#if NIKON_DEBUG
            ESP_LOGI(LOG_TAG, "data(stage) = %s",
                     NimBLEUtils::dataToHexString(pData, length).c_str());
#endif
            bool rc = this->processStage(pData, length);
            if (!rc) {
              ESP_LOGI(LOG_TAG, "Stage %u response mismatch", this->getStage());
            }
            xQueueSend(m_Queue, &rc, 0);
          },
          true)) {
    return false;
  }

#if 0
  // perform four stage handshake
  for (int i = 0; i < 100; i++) {
    pair_msg_t msg = {0x01, 0x00, 0xa5a5a5a5a5a5a5a5};
    if (!m_Client->setValue(SERVICE_UUID, STAGE_CHR_UUID,
                            {(const uint8_t *)&msg, (uint16_t)sizeof(msg)}, true)) {
      return false;
    }
#if NIKON_DEBUG
    ESP_LOGI(LOG_TAG, "sent = %s",
             NimBLEUtils::dataToHexString((const uint8_t *)&msg, sizeof(msg)).c_str());
#endif

    vTaskDelay(pdMS_TO_TICKS(200));
  }
#else
  // perform four stage handshake
  m_PairMsg = {0x00, 0x00, 0x00};
  processStage(reinterpret_cast<uint8_t *>(&m_PairMsg), sizeof(m_PairMsg));
  for (; stage < 4 && !failed; stage += 2) {
    if (!m_PairChr->writeValue((const uint8_t *)&m_PairMsg, sizeof(m_PairMsg), true)) {
      return false;
    }
#if NIKON_DEBUG
    ESP_LOGI(LOG_TAG, "sent = %s",
             NimBLEUtils::dataToHexString((const uint8_t *)&m_PairMsg,
                                          sizeof(m_PairMsg))
                 .c_str());
#endif

    // wait 10s for a notification
    BaseType_t timeout = xQueueReceive(m_Queue, &success, pdMS_TO_TICKS(10000));
    if (timeout == pdFALSE) {
      success = false;
    }
    m_Progress += 5;
  }
#endif

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

bool Nikon::processStage(const uint8_t *data, const size_t length) {
  if (length != sizeof(pair_msg_t)) {
    return false;
  }

  pair_msg_t msg;
  memcpy(&msg, data, length);

  switch (msg.stage) {
    case 0:
      m_PairMsg = {0x01, 0x00, 0x00};
      return true;
    case 2:
      {
        pair_msg_t expected = {0x02, 0x00, 0x00};
        if (memcmp(&msg, &expected, sizeof(msg)) == 0) {
          m_PairMsg = { 0x03, 0x00, 0x00 };
          return true;
        }
      }
      break;
    case 4:
      {
        pair_msg_t expected = {0x04, 0x00, 0x00};
        if (memcmp(&msg, &expected, sizeof(msg)) == 0) {
          m_PairMsg = { 0x05, 0x00, 0x00 };
          return true;
        }
      }
      break;
  }

  return false;
}

uint8_t Nikon::getStage(void) {
  return m_PairMsg.stage;
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
      .extras = (uint16_t)gps.satellites,
      .altitude = (uint16_t)gps.altitude,
      .time = ntime,
      .subseconds = (uint8_t)timesync.centisecond,
      .valid = 0x01,
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

void Nikon::bf_hash(uint32_t *src, uint32_t *dest, uint16_t length) {
  uint32_t right = 0x05060708;
  uint32_t left = 0x01020304;
  uint32_t inL = 0;
  uint32_t inR = 0;

  for (uint16_t i = 0; i < length; i+= 2) {
    inL = src[i] ^ left;
    inR = src[i+1] ^ right;
    bf_nikon_scramble(&inL, &inR);
    left = inL;
    right = inR;
  }

  dest[0] = inL;
  dest[1] = inR;
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
