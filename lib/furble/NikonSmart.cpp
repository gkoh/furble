#include <cmath>
#include <cstring>

#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "Device.h"
#include "NikonSmart.h"

namespace Furble {

const std::vector<uint8_t> NikonSmart::SmartPairing::KEY = {0xff, 0xff, 0xaa, 0x55,
                                                            0x11, 0x22, 0x33, 0x00};
const std::array<std::array<uint32_t, 2>, 8> NikonSmart::SmartPairing::SALT = {
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
const NimBLEUUID NikonSmart::PAIR_CHR_UUID {0x00002000, 0x3dd4, 0x4255, 0x8d626dc7b9bd5561};
constexpr std::array<uint8_t, 2> NikonSmart::SUCCESS;

NikonSmart::SmartPairing::SmartPairing(const uint64_t timestamp, const NikonBase::Pairing::id_t id)
    : NikonBase::Pairing(timestamp, id), Blowfish(KEY) {
  m_Stage[2].timestamp = timestamp;
}

void NikonSmart::SmartPairing::scramble(uint32_t *pL, uint32_t *pR) const {
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

std::array<uint32_t, 2> NikonSmart::SmartPairing::hash(const uint32_t *src, size_t len) const {
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

int8_t NikonSmart::SmartPairing::findSaltIndex(const msg_t &msg) {
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

const NikonBase::Pairing::msg_t *NikonSmart::SmartPairing::processMessage(const msg_t &msg) {
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

NikonSmart::NikonSmart(NimBLEClient *client,
                       QueueHandle_t queue,
                       NimBLERemoteCharacteristic *pairChr,
                       const NikonBase::Pairing::id_t &id,
                       const uint64_t timestamp,
                       std::atomic<uint8_t> *progress)
    : NikonBase(client, queue, pairChr, progress) {
  m_Pairing = std::make_unique<SmartPairing>(timestamp, id);
}

bool NikonSmart::preSubscribe(NimBLERemoteService *pSvc) {
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
  return true;
}

bool NikonSmart::connectFinalise(void) {
  bool success = false;
  // wait for final OK
  BaseType_t timeout = xQueueReceive(m_Queue, &success, pdMS_TO_TICKS(10000));
  if (timeout == pdFALSE) {
    success = false;
    ESP_LOGI(LOG_TAG, "Timeout waiting for final OK.");
  }

  if (!success) {
    return false;
  }

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
}

void NikonSmart::shutterPress(void) {
  // do nothing
}

void NikonSmart::shutterRelease(void) {
  // do nothing
}

void NikonSmart::focusPress(void) {
  // do nothing
}

void NikonSmart::focusRelease(void) {
  // do nothing
}

void NikonSmart::updateGeoData(const Camera::gps_t &gps, const Camera::timesync_t &timesync) {
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
      .latitude_submin1 = 0,
      .latitude_submin2 = 0,
      .longitude_direction = gps.longitude < 0.0 ? 'W' : 'E',
      .longitude_degrees = 0,
      .longitude_minutes = 0,
      .longitude_submin1 = 0,
      .longitude_submin2 = 0,
      .satellites = static_cast<uint8_t>(gps.satellites),
      .altitude_ref = gps.altitude < 0.0 ? 'M' : 'P',
      .altitude = (uint16_t)gps.altitude,
      .time = ntime,
      .subseconds = (uint8_t)timesync.centisecond,
      .valid = 0x01,
      .standard = {'W', 'G', 'S', '-', '8', '4'},
      .pad = {0x00},
  };

  degreesToDMSubMin(gps.latitude, geo.latitude_degrees, geo.latitude_minutes, geo.latitude_submin1,
                    geo.latitude_submin2);
  degreesToDMSubMin(gps.longitude, geo.longitude_degrees, geo.longitude_minutes,
                    geo.longitude_submin1, geo.longitude_submin2);

  ESP_LOGI(LOG_TAG, "sending GPS = %s",
           NimBLEUtils::dataToHexString((const uint8_t *)&geo, sizeof(geo)).c_str());
  if (m_Client->setValue(SERVICE_UUID, GEO_CHR_UUID, {(const uint8_t *)&geo, (uint16_t)sizeof(geo)},
                         true)) {
    ESP_LOGI(LOG_TAG, "  success");
  } else {
    ESP_LOGI(LOG_TAG, "  failed");
  }
}

// Converts a decimal-degree value into the Nikon encoding:
// degrees, whole minutes, and two "sub-minute" bytes that
// together represent the fractional minutes as a 4-digit number
// (submin1 = hundredths of a minute, submin2 = hundredths of the
// remainder).
void NikonSmart::degreesToDMSubMin(double value,
                                   uint8_t &degrees,
                                   uint8_t &minutes,
                                   uint8_t &submin1,
                                   uint8_t &submin2) {
  double integral;
  double absValue = std::fabs(value);

  // Whole degrees (truncated, matching Math.floor on a non-negative value).
  std::modf(absValue, &integral);
  degrees = (uint8_t)integral;

  // Remaining fractional degrees, converted to minutes.
  double minutesFull = (absValue - degrees) * 60.0;
  std::modf(minutesFull, &integral);
  minutes = (uint8_t)integral;

  // Remaining fractional minutes, scaled by 100
  double subMinFull = (minutesFull - minutes) * 100.0;
  std::modf(subMinFull, &integral);
  submin1 = (uint8_t)integral;

  // Remaining fractional hundredths-of-a-minute, scaled by 100 again.
  double subMin2Full = (subMinFull - submin1) * 100.0;
  std::modf(subMin2Full, &integral);
  submin2 = (uint8_t)integral;
}

}  // namespace Furble
