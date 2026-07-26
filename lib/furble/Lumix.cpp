#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "Device.h"
#include "Lumix.h"

namespace Furble {

const NimBLEUUID Lumix::SESSION_SVC_UUID {0x054ac620, 0x3214, 0x11e6, 0xac0d0002a5d5c51b};
const NimBLEUUID Lumix::SESSION_INIT_UUID {0x8d08a420, 0x3213, 0x11e6, 0x8aca0002a5d5c51b};
const NimBLEUUID Lumix::DEVICE_NAME_UUID {0xcd7a71a0, 0x3213, 0x11e6, 0x8f560002a5d5c51b};
const NimBLEUUID Lumix::CONTROL_SVC_UUID {0x7be5faae, 0x475b, 0x11e7, 0xa91992ebcb67fe33};
const NimBLEUUID Lumix::CONTROL_UUID {0x7be5fd56, 0x475b, 0x11e7, 0xa91992ebcb67fe33};
const NimBLEUUID Lumix::LOCATION_SVC_UUID {0x1d74afe0, 0x3214, 0x11e6, 0x8ab40002a5d5c51b};
const NimBLEUUID Lumix::LOCATION_CHR_UUID {0xdaff1bc0, 0x3216, 0x11e6, 0x91c80002a5d5c51b};
const NimBLEUUID Lumix::CLOCK_SVC_UUID {0x34738720, 0x3214, 0x11e6, 0xb66b0002a5d5c51b};
const NimBLEUUID Lumix::CLOCK_CHR_UUID {0xead55e60, 0x3216, 0x11e6, 0xa42e0002a5d5c51b};

// Seconds from 1970-01-01 (Unix epoch) to 1980-01-06 (GPS epoch).
static constexpr uint32_t GPS_EPOCH_OFFSET = 315964800;

Lumix::Lumix(const void *data, size_t len) : Camera(Type::PANASONIC_LUMIX, PairType::SAVED) {
  if (len != sizeof(lumix_t))
    abort();

  const lumix_t *lumix = static_cast<const lumix_t *>(data);
  m_Name = std::string(lumix->name);
  m_Address = NimBLEAddress(lumix->address, lumix->type);
}

Lumix::Lumix(const NimBLEAdvertisedDevice *pDevice) : Camera(Type::PANASONIC_LUMIX, PairType::NEW) {
  m_Name = pDevice->getName();
  if (m_Name.empty())
    m_Name = "LUMIX";
  m_Address = pDevice->getAddress();
  ESP_LOGI(LOG_TAG, "Lumix Name = %s", m_Name.c_str());
  ESP_LOGI(LOG_TAG, "Lumix Address = %s", m_Address.toString().c_str());
}

bool Lumix::matches(const NimBLEAdvertisedDevice *pDevice) {
  if (pDevice->haveManufacturerData() && pDevice->getManufacturerData().size() >= sizeof(adv_t)) {
    const adv_t adv = pDevice->getManufacturerData<adv_t>();
    if (adv.company_id != ADV_PANASONIC_ID)
      return false;
  } else {
    return false;
  }
  return pDevice->isAdvertisingService(SESSION_SVC_UUID);
}

bool Lumix::_connect(void) {
  m_Progress = 10;
  ESP_LOGI(LOG_TAG, "Lumix connecting");
  if (!m_Client->connect(m_Address)) {
    ESP_LOGI(LOG_TAG, "Lumix connection failed");
    return false;
  }
  m_Progress = 30;

  // MEI0 handshake
  NimBLERemoteService *pSvc = m_Client->getService(SESSION_SVC_UUID);
  if (pSvc == nullptr) {
    ESP_LOGI(LOG_TAG, "Lumix session service unavailable");
    return false;
  }

  NimBLERemoteCharacteristic *pChr = pSvc->getCharacteristic(SESSION_INIT_UUID);
  if (pChr == nullptr || !pChr->canWrite()) {
    ESP_LOGI(LOG_TAG, "Lumix session-init characteristic unavailable");
    return false;
  }
  if (!pChr->writeValue(MEI0.data(), MEI0.size(), true)) {
    ESP_LOGI(LOG_TAG, "Lumix MEI0 write failed");
    return false;
  }
  m_Progress = 50;

  // Device name
  pChr = pSvc->getCharacteristic(DEVICE_NAME_UUID);
  if (pChr == nullptr || !pChr->canWrite()) {
    ESP_LOGI(LOG_TAG, "Lumix device-name characteristic unavailable");
    return false;
  }
  // Send identity
  const std::string name = Device::getStringID();
  if (!pChr->writeValue(name.c_str(), name.length(), true)) {
    ESP_LOGI(LOG_TAG, "Lumix device-name write failed");
    return false;
  }
  m_Progress = 70;

  // Remote control service.
  pSvc = m_Client->getService(CONTROL_SVC_UUID);
  if (pSvc == nullptr) {
    ESP_LOGI(LOG_TAG, "Lumix control service unavailable");
    return false;
  }
  m_Control = pSvc->getCharacteristic(CONTROL_UUID);
  if (m_Control == nullptr || !m_Control->canWrite()) {
    ESP_LOGI(LOG_TAG, "Lumix control characteristic unavailable");
    return false;
  }

  // Location and clock services (optional).
  pSvc = m_Client->getService(LOCATION_SVC_UUID);
  if (pSvc != nullptr) {
    m_Location = pSvc->getCharacteristic(LOCATION_CHR_UUID);
  } else {
    ESP_LOGI(LOG_TAG, "Lumix location service unavailable");
  }
  pSvc = m_Client->getService(CLOCK_SVC_UUID);
  if (pSvc != nullptr) {
    m_Clock = pSvc->getCharacteristic(CLOCK_CHR_UUID);
  } else {
    ESP_LOGI(LOG_TAG, "Lumix clock service unavailable");
  }

  m_Progress = 100;
  ESP_LOGI(LOG_TAG, "Lumix connected");
  return true;
}

void Lumix::_disconnect(void) {
  m_Control = nullptr;
  m_Location = nullptr;
  m_Clock = nullptr;
  if (m_Client != nullptr && m_Client->isConnected())
    m_Client->disconnect();
}

bool Lumix::writeCommand(uint8_t command) {
  if (!isConnected() || m_Control == nullptr || !m_Control->canWrite()) {
    ESP_LOGW(LOG_TAG, "Lumix control write skipped: command=0x%02x connected=%d control=%p",
             command, isConnected(), m_Control);
    return false;
  }
  if (!m_Control->writeValue(&command, sizeof(command), true)) {
    ESP_LOGW(LOG_TAG, "Lumix control write failed: command=0x%02x", command);
    return false;
  }
  return true;
}

void Lumix::shutterPress(void) {
  writeCommand(CMD_SHUTTER_PRESS);
}

void Lumix::shutterRelease(void) {
  writeCommand(CMD_SHUTTER_RELEASE);
}

void Lumix::focusPress(void) {
  writeCommand(CMD_FOCUS_PRESS);
}

void Lumix::focusRelease(void) {
  writeCommand(CMD_FOCUS_RELEASE);
}

void Lumix::updateGeoData(const gps_t &gps, const timesync_t &timesync) {
  if (!isConnected()) {
    ESP_LOGW(LOG_TAG, "Lumix GPS skipped: not connected");
    return;
  }
  if (m_Location == nullptr || !m_Location->canWrite()) {
    ESP_LOGW(LOG_TAG, "Lumix GPS characteristic unavailable");
    return;
  }

  geo_t geo = {
      .gps_time = static_cast<uint32_t>(toUnixTime(timesync)) - GPS_EPOCH_OFFSET,
      .latitude = static_cast<int32_t>(gps.latitude * 10000000.0),
      .longitude = static_cast<int32_t>(gps.longitude * 10000000.0),
      .altitude = static_cast<uint16_t>(gps.altitude),
      .fix = 0x41,  // 'A' — NMEA valid fix
      .reserved = 0x00,
  };
  bool rc = m_Location->writeValue(reinterpret_cast<const uint8_t *>(&geo), sizeof(geo), true);

  // Synchronise the camera clock when a write characteristic is available.
  if (rc && m_Clock != nullptr && m_Clock->canWrite()) {
    clock_t clock = {
        .year = static_cast<uint16_t>(timesync.year),
        .month = static_cast<uint8_t>(timesync.month),
        .day = static_cast<uint8_t>(timesync.day),
        .hour = static_cast<uint8_t>(timesync.hour),
        .minute = static_cast<uint8_t>(timesync.minute),
        .second = static_cast<uint8_t>(timesync.second),
        .zero = 0x00,
        .tz_offset = 0,
    };
    m_Clock->writeValue(reinterpret_cast<const uint8_t *>(&clock), sizeof(clock), true);
  }

  ESP_LOGI(LOG_TAG, "Lumix GPS lat=%.7f lon=%.7f alt=%.1f utc=%04u-%02u-%02u %02u:%02u:%02u => %s",
           gps.latitude, gps.longitude, gps.altitude, timesync.year, timesync.month, timesync.day,
           timesync.hour, timesync.minute, timesync.second, rc ? "ok" : "failed");
}

size_t Lumix::getSerialisedBytes(void) const {
  return sizeof(lumix_t);
}

bool Lumix::serialise(void *buffer, size_t bytes) const {
  if (bytes != sizeof(lumix_t))
    return false;
  lumix_t *x = static_cast<lumix_t *>(buffer);
  strncpy(x->name, m_Name.c_str(), MAX_NAME);
  x->name[MAX_NAME - 1] = '\0';
  x->address = static_cast<uint64_t>(m_Address);
  x->type = m_Address.getType();
  return true;
}

}  // namespace Furble
