#include <cstring>

#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include <esp_random.h>

#include "Nikon.h"
#include "NikonRemote.h"
#include "NikonSmart.h"
#include "Scan.h"

namespace Furble {

Nikon::Nikon(const void *data, size_t len) : Camera(Type::NIKON, PairType::SAVED) {
  if (len != sizeof(nikon_t))
    abort();

  const nikon_t *nikon = static_cast<const nikon_t *>(data);
  m_Name = std::string(nikon->name);
  m_Address = NimBLEAddress(nikon->address, nikon->type);
  esp_fill_random(&m_Timestamp, sizeof(m_Timestamp));
  std::memcpy(&m_ID, &nikon->id, sizeof(m_ID));
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
  return (pDevice->haveServiceUUID() && (pDevice->getServiceUUID() == NikonBase::SERVICE_UUID));
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

  if (m_PairType == PairType::SAVED || m_Paired) {
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

  auto *pSvc = m_Client->getService(NikonBase::SERVICE_UUID);
  if (pSvc == nullptr) {
    return false;
  }

  // Try as remote first, then smart device. The pair characteristic found
  // determines remote vs smart.
  auto *pairChr = pSvc->getCharacteristic(NikonRemote::REMOTE_PAIR_CHR_UUID);
  if (pairChr == nullptr) {
    pairChr = pSvc->getCharacteristic(NikonSmart::PAIR_CHR_UUID);
    if (pairChr == nullptr) {
      return false;
    }
    m_Nikon =
        std::make_unique<NikonSmart>(m_Client, m_Queue, pairChr, m_ID, m_Timestamp, &m_Progress);
  } else {
    m_Nikon = std::make_unique<NikonRemote>(m_Client, m_Queue, pairChr, m_ID, &m_Progress);
  }

  return m_Nikon->connect(pSvc);
}

void Nikon::_disconnect(void) {
  m_Client->disconnect();
}

void Nikon::shutterPress(void) {
  m_Nikon->shutterPress();
}

void Nikon::shutterRelease(void) {
  m_Nikon->shutterRelease();
}

void Nikon::focusPress(void) {
  m_Nikon->focusPress();
}

void Nikon::focusRelease(void) {
  m_Nikon->focusRelease();
}

void Nikon::updateGeoData(const gps_t &gps, const timesync_t &timesync) {
  m_Nikon->updateGeoData(gps, timesync);
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
  std::memcpy(&x->id, &m_ID, sizeof(x->id));

  return true;
}

}  // namespace Furble
