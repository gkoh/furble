#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include <esp_random.h>

#include "Blowfish.h"
#include "Device.h"
#include "Nikon.h"

#define NIKON_DEBUG (1)

namespace Furble {

const std::vector<uint8_t> Nikon::SmartPairing::KEY = {0xff, 0xff, 0xaa, 0x55,
                                                       0x11, 0x22, 0x33, 0x00};
const std::array<std::array<uint32_t, 2>, 8> Nikon::SmartPairing::SALT = {
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

Nikon::Pairing::Pairing(const Pairing::Type type, const uint64_t timestamp, const id_t id)
    : m_Type(type) {
  m_Stage[0].stage = 0x01;
  m_Stage[0].timestamp = timestamp;
  m_Stage[0].id = id;
  m_Stage[1].stage = 0x02;
  m_Stage[2].stage = 0x03;
  m_Stage[3].stage = 0x04;
  m_Stage[4].stage = 0x05;

  m_Msg = &m_Stage[0];
};

const Nikon::Pairing::msg_t *Nikon::Pairing::getMessage(void) const {
  return m_Msg;
}

Nikon::Pairing::Type Nikon::Pairing::getType(void) const {
  return m_Type;
}

Nikon::RemotePairing::RemotePairing(const uint64_t timestamp, const Pairing::id_t &id)
    : Pairing(Type::REMOTE, timestamp, id) {
  m_Stage[1].timestamp = 0x00;
  m_Stage[1].id = {0x00, 0x00};
  m_Stage[2].timestamp = 0x00;
  m_Stage[2].id = {0x00, 0x00};
  m_Stage[3].timestamp = 0x00;
  m_Stage[3].id = {0x00, 0x00};
  m_Stage[4].timestamp = 0x00;
  m_Stage[4].id = {0x00, 0x00};
};

const Nikon::Pairing::msg_t *Nikon::RemotePairing::processMessage(const msg_t &msg) {
  switch (msg.stage) {
    case 0:
      m_Msg = &m_Stage[0];
      return m_Msg;
    case 2:
    {
      const msg_t *expected = &m_Stage[1];
      if (memcmp(&msg, expected, sizeof(msg)) == 0) {
        m_Msg = &m_Stage[2];
        return m_Msg;
      }
    } break;
    case 4:
    {
      if (msg.timestamp == m_Stage[3].timestamp) {
        char serial[sizeof(msg.serial) + 1] = {0x00};
        strncpy(serial, msg.serial, sizeof(serial) - 1);

        ESP_LOGI(LOG_TAG, "Serial: %s", serial);
        m_Msg = &m_Stage[4];
        return m_Msg;
      }
    } break;
  }

  return nullptr;
}

Nikon::SmartPairing::SmartPairing(const uint64_t timestamp, const id_t id)
    : Pairing(Type::SMART_DEVICE, timestamp, id), Blowfish(KEY) {
  m_Stage[2].timestamp = timestamp;
};

void Nikon::SmartPairing::scramble(uint32_t *pL, uint32_t *pR) const {
  uint32_t xL = *pL;
  uint32_t xR = *pR;
  uint32_t tmp = 0;

  for (int i = 0; i < N; i++) {
    tmp = xL ^ m_P[i];
    xL = f(tmp) ^ xR;
    xR = tmp;
  }

  *pL = tmp ^ m_P[N + 1];
  *pR = xL ^ m_P[N];
}

std::array<uint32_t, 2> Nikon::SmartPairing::hash(const uint32_t *src, size_t len) const {
  uint32_t right = 0x05060708;
  uint32_t left = 0x01020304;
  uint32_t inL = 0;
  uint32_t inR = 0;

  for (uint16_t i = 0; i < len; i += 2) {
    inL = src[i] ^ left;
    inR = src[i + 1] ^ right;
    scramble(&inL, &inR);
    left = inL;
    right = inR;
  }

  return std::array<uint32_t, 2> {inL, inR};
}

int8_t Nikon::SmartPairing::findSaltIndex(const msg_t &msg) {
  for (size_t i = 0; i < SALT.size(); i++) {
    std::array<uint32_t, 6> s = {SALT[i][0],
                                 SALT[i][1],
                                 __builtin_bswap32(msg.timestampH),
                                 __builtin_bswap32(msg.timestampL),
                                 __builtin_bswap32(m_Stage[0].timestampH),
                                 __builtin_bswap32(m_Stage[0].timestampL)};
    std::array<uint32_t, 2> s2 = hash(s.data(), s.size());
    if ((s2[0] == __builtin_bswap32(msg.id.device) && (s2[1] == __builtin_bswap32(msg.id.nonce)))) {
      return i;
    }
  }
  return -1;
}

const Nikon::Pairing::msg_t *Nikon::SmartPairing::processMessage(const msg_t &msg) {
  switch (msg.stage) {
    case 0:
      m_Msg = &m_Stage[0];
      return m_Msg;
    case 2:
    {
      m_Salt = findSaltIndex(msg);
      if (m_Salt >= 0) {
        std::array<uint32_t, 6> buffer = {SALT[m_Salt][0],
                                          SALT[m_Salt][1],
                                          __builtin_bswap32(m_Stage[0].timestampH),
                                          __builtin_bswap32(m_Stage[0].timestampL),
                                          __builtin_bswap32(msg.timestampH),
                                          __builtin_bswap32(msg.timestampL)};
        std::array<uint32_t, 2> s3 = hash(buffer.data(), buffer.size());
        m_Stage[2].id = {__builtin_bswap32(s3[0]), __builtin_bswap32(s3[1])};
        m_Msg = &m_Stage[2];

        m_Stage[3].timestamp = msg.timestamp;

        return m_Msg;
      }
    } break;
    case 4:
    {
      if (msg.timestamp == m_Stage[3].timestamp) {
        char serial[sizeof(msg.serial) + 1] = {0x00};
        strncpy(serial, msg.serial, sizeof(serial) - 1);

        ESP_LOGI(LOG_TAG, "Serial: %s", serial);
        m_Msg = &m_Stage[4];
        return m_Msg;
      }
    } break;
  }

  return nullptr;
}

Nikon::Nikon(const void *data, size_t len) : Camera(Type::NIKON, PairType::SAVED) {
  if (len != sizeof(nikon_t))
    abort();

  const nikon_t *nikon = static_cast<const nikon_t *>(data);
  m_Name = std::string(nikon->name);
  m_Address = NimBLEAddress(nikon->address, nikon->type);
  esp_fill_random(&m_Timestamp, sizeof(m_Timestamp));
  memcpy(&m_ID, &nikon->id, sizeof(m_ID));
  m_Queue = xQueueCreate(3, sizeof(bool));
}

Nikon::Nikon(const NimBLEAdvertisedDevice *pDevice) : Camera(Type::NIKON, PairType::NEW) {
  m_Name = pDevice->getName();
  m_Address = pDevice->getAddress();
  esp_fill_random(&m_Timestamp, sizeof(m_Timestamp));
  esp_fill_random(&m_ID, sizeof(m_ID));
  // remote mode device ID always seems to start with 0x01
  m_ID.device &= __builtin_bswap32(0x00ffffff);
  m_ID.device |= __builtin_bswap32(0x01000000);
  m_Queue = xQueueCreate(3, sizeof(bool));
}

Nikon::~Nikon(void) {
  vQueueDelete(m_Queue);
}

bool Nikon::matchesServiceUUID(const NimBLEAdvertisedDevice *pDevice) {
  return (pDevice->haveServiceUUID() && (pDevice->getServiceUUID() == SERVICE_UUID));
}

/**
 * Determine if the advertised BLE device is a Nikon.
 *
 * During remote pairing, the camera appears to:
 * * advertise the service UUID
 * * have no manufacturer data
 */
bool Nikon::matches(const NimBLEAdvertisedDevice *pDevice) {
  return (!pDevice->haveManufacturerData() && matchesServiceUUID(pDevice));
}

void Nikon::onResult(const NimBLEAdvertisedDevice *pDevice) {
  if (pDevice->haveManufacturerData() && matchesServiceUUID(pDevice)) {
    nikon_adv_t saved = {COMPANY_ID, m_ID.device, 0x00};
    nikon_adv_t found = pDevice->getManufacturerData<nikon_adv_t>();

    if ((saved.companyID == found.companyID) && (saved.device == found.device)) {
      m_Address = pDevice->getAddress();
      bool success = true;
      xQueueSend(m_Queue, &success, 0);
    }
  }
}

bool Nikon::_connect(void) {
  bool success = false;
  m_Progress = 0;

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

  auto *pSvc = m_Client->getService(SERVICE_UUID);
  if (pSvc == nullptr) {
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
    m_Pairing = new SmartPairing(m_Timestamp, m_ID);
    ESP_LOGI(LOG_TAG, "Connecting as smart device, subscribing to success notification");
    auto *pChr = pSvc->getCharacteristic(NOT1_CHR_UUID);
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
    ESP_LOGI(LOG_TAG, "Subscribed to success notification!");
  } else {
    // Remote timestamp always seems to be 0x01
    m_Pairing = new RemotePairing(__builtin_bswap64(0x01), m_ID);
    ESP_LOGI(LOG_TAG, "Connecting as remote, subscribing to indication 1");
    auto *pChr = pSvc->getCharacteristic(REMOTE_IND1_CHR_UUID);
    if (!pChr->subscribe(
            false,
            [this](NimBLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData,
                   size_t length, bool isNotify) {
#if NIKON_DEBUG
              ESP_LOGI(LOG_TAG, "data(ind1) = %s",
                       NimBLEUtils::dataToHexString(pData, length).c_str());
#endif
            },
            true)) {
      return false;
    }
    ESP_LOGI(LOG_TAG, "Subscribed to indication 1!");
  }

  ESP_LOGI(LOG_TAG, "Subscribing to pairing indication");
  if (!m_PairChr->subscribe(
          false,
          [this](NimBLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData,
                 size_t length, bool isNotify) {
#if NIKON_DEBUG
            ESP_LOGI(LOG_TAG, "data(stage) = %s",
                     NimBLEUtils::dataToHexString(pData, length).c_str());
#endif
            Nikon::Pairing::msg_t msg;
            memcpy(&msg, pData, sizeof(msg));
            auto pMsg = this->m_Pairing->processMessage(msg);
            bool rc = (pMsg != nullptr);
            if (!rc) {
              ESP_LOGI(LOG_TAG, "Stage response mismatch");
            }
            xQueueSend(m_Queue, &rc, 0);
          },
          true)) {
    return false;
  }
  ESP_LOGI(LOG_TAG, "Subscribed to pairing indication!");

  success = true;

  // perform four stage handshake
  for (uint8_t stage = 0; stage < 4 && success; stage += 2) {
    const auto *msg = m_Pairing->getMessage();
    if (!m_PairChr->writeValue((const uint8_t *)msg, sizeof(*msg), true)) {
      return false;
    }
#if NIKON_DEBUG
    ESP_LOGI(LOG_TAG, "sent = %s",
             NimBLEUtils::dataToHexString((const uint8_t *)msg, sizeof(*msg)).c_str());
#endif

    // wait 10s for a notification
    BaseType_t timeout = xQueueReceive(m_Queue, &success, pdMS_TO_TICKS(10000));
    if (timeout == pdFALSE) {
      success = false;
      ESP_LOGI(LOG_TAG, "Timeout waiting for stage response.");
      break;
    }

    m_Progress += 5;
  }

  if (!success) {
    return false;
  }

  m_Progress += 10;

  if (m_Pairing->getType() == Pairing::Type::SMART_DEVICE) {
    // wait for final OK
    BaseType_t timeout = xQueueReceive(m_Queue, &success, pdMS_TO_TICKS(10000));
    if (timeout == pdFALSE) {
      success = false;
      ESP_LOGI(LOG_TAG, "Timeout waiting for final OK.");
    }
  } else {
    success = true;
  }

  if (!success) {
    return false;
  }

  if (m_Pairing->getType() == Pairing::Type::SMART_DEVICE) {
    const auto name = Device::getStringID();
    ESP_LOGI(LOG_TAG, "Identifying as %s", name.c_str());
    if (!m_Client->setValue(SERVICE_UUID, ID_CHR_UUID, name, true)) {
      return false;
    }

    // Unable to continue at this time
    // For some reason Nikon smart device pairing swaps to Bluetooth Classic to
    // establish secure bond and our Bluetooth stack is LE only.
    ESP_LOGI(LOG_TAG, "Nikon smart device pairing not fully functional");

    return false;
  } else {
    // nothing more needed for remote mode
  }

  ESP_LOGI(LOG_TAG, "%s", success ? "Done!" : "Failed to receive final OK.");

  m_Progress = 100;

  return success;
}

void Nikon::shutterPress(void) {
  std::array<uint8_t, 2> cmd = {MODE_SHUTTER, CMD_PRESS};
  m_Client->setValue(SERVICE_UUID, REMOTE_SHUTTER_CHR_UUID, {cmd.data(), cmd.size()}, true);
}

void Nikon::shutterRelease(void) {
  std::array<uint8_t, 2> cmd = {MODE_SHUTTER, CMD_RELEASE};
  m_Client->setValue(SERVICE_UUID, REMOTE_SHUTTER_CHR_UUID, {cmd.data(), cmd.size()}, true);
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
