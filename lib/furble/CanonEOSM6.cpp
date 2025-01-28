#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "CanonEOSM6.h"

namespace Furble {

constexpr uint8_t CanonEOSM6::MODE_SHOOT;

bool CanonEOSM6::matches(const NimBLEAdvertisedDevice *pDevice) {
  if (pDevice->haveManufacturerData() && pDevice->getManufacturerData().length() == ADV_DATA_LEN) {
    const char *data = pDevice->getManufacturerData().data();
    if (data[0] == ID_0 && data[1] == ID_1 && data[2] == XX_2 && data[3] == XX_3
        && data[4] == XX_4) {
      // All remaining bits should be zero.
      uint8_t zero = 0;
      for (size_t i = 5; i < ADV_DATA_LEN; i++) {
        zero |= data[i];
      }
      return (zero == 0);
    }
  }
  return false;
}

// Handle pairing notification
void CanonEOSM6::pairCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                              uint8_t *pData,
                              size_t length,
                              bool isNotify) {
  if (!isNotify && (length > 0)) {
    ESP_LOGI(LOG_TAG, "Got pairing callback: 0x%02x", pData[0]);
    m_PairResult = pData[0];
  }
}

bool CanonEOSM6::write_prefix(const NimBLEUUID &serviceUUID,
                              const NimBLEUUID &characteristicUUID,
                              const uint8_t prefix,
                              const uint8_t *data,
                              uint16_t length) {
  uint8_t buffer[length + 1] = {0};
  buffer[0] = prefix;
  memcpy(&buffer[1], data, length);
  return m_Client->setValue(serviceUUID, characteristicUUID, {&buffer[0], (uint16_t)(length + 1)});
}

/**
 * Connect to a Canon EOSM6.
 *
 * The EOS uses the 'just works' BLE bonding to pair, all bond management is
 * handled by the underlying NimBLE and ESP32 libraries.
 */
bool CanonEOSM6::_connect(void) {
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

  NimBLERemoteService *pSvc = m_Client->getService(SVC_IDEN_UUID);
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
  if (!write_prefix(SVC_IDEN_UUID, CHR_NAME_UUID, 0x01, (uint8_t *)name.c_str(), name.length()))
    return false;

  m_Progress = 30;

  ESP_LOGI(LOG_TAG, "Identifying 2!");
  if (!write_prefix(SVC_IDEN_UUID, CHR_IDEN_UUID, 0x03, m_Uuid.uint8, Device::UUID128_LEN))
    return false;

  m_Progress = 40;

  ESP_LOGI(LOG_TAG, "Identifying 3!");
  if (!write_prefix(SVC_IDEN_UUID, CHR_IDEN_UUID, 0x04, (uint8_t *)name.c_str(), name.length()))
    return false;

  m_Progress = 50;

  ESP_LOGI(LOG_TAG, "Identifying 4!");

  std::array<uint8_t, 1> x = {0x02};
  if (!write_prefix(SVC_IDEN_UUID, CHR_IDEN_UUID, 0x05, x.data(), x.size())) {
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
  if (!m_Client->setValue(SVC_IDEN_UUID, CHR_IDEN_UUID, {x.data(), x.size()}))
    return false;

  m_Progress = 90;

  ESP_LOGI(LOG_TAG, "Switching mode!");

  /* write to 0xf307 */
  if (!m_Client->setValue(SVC_MODE_UUID, CHR_MODE_UUID, {&MODE_SHOOT, sizeof(MODE_SHOOT)}))
    return false;

  ESP_LOGI(LOG_TAG, "Done!");
  m_Progress = 100;

  return true;
}

void CanonEOSM6::shutterPress(void) {
  const std::array<uint8_t, 2> x = {0x00, 0x01};
  m_Client->setValue(SVC_SHUTTER_UUID, CHR_SHUTTER_UUID, {x.data(), x.size()});
}

void CanonEOSM6::shutterRelease(void) {
  const std::array<uint8_t, 2> x = {0x00, 0x02};
  m_Client->setValue(SVC_SHUTTER_UUID, CHR_SHUTTER_UUID, {x.data(), x.size()});
}

void CanonEOSM6::focusPress(void) {
  // do nothing
  return;
}

void CanonEOSM6::focusRelease(void) {
  // do nothing
  return;
}

void CanonEOSM6::updateGeoData(const gps_t &gps, const timesync_t &timesync) {
  // do nothing
  return;
}

}  // namespace Furble
