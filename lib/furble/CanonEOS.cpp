#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "CanonEOS.h"
#include "Device.h"

namespace Furble {

CanonEOS::CanonEOS(Type type, const void *data, size_t len) : Camera(type) {
  if (len != sizeof(eos_t))
    abort();

  const eos_t *eos = static_cast<const eos_t *>(data);
  m_Name = std::string(eos->name);
  m_Address = NimBLEAddress(eos->address, eos->type);
  memcpy(&m_Uuid, &eos->uuid, sizeof(Device::uuid128_t));
}

CanonEOS::CanonEOS(Type type, const NimBLEAdvertisedDevice *pDevice) : Camera(type) {
  m_Name = pDevice->getName();
  m_Address = pDevice->getAddress();
  ESP_LOGI(LOG_TAG, "Name = %s", m_Name.c_str());
  ESP_LOGI(LOG_TAG, "Address = %s", m_Address.toString().c_str());
  Device::getUUID128(&m_Uuid);
}

// Handle pairing notification
void CanonEOS::pairCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                            uint8_t *pData,
                            size_t length,
                            bool isNotify) {
  if (!isNotify && (length > 0)) {
    ESP_LOGI(LOG_TAG, "Got pairing callback: 0x%02x", pData[0]);
    m_PairResult = pData[0];
  }
}

bool CanonEOS::write_value(NimBLEClient *pClient,
                           const char *serviceUUID,
                           const char *characteristicUUID,
                           uint8_t *data,
                           size_t length) {
  NimBLERemoteService *pSvc = pClient->getService(serviceUUID);
  if (pSvc) {
    NimBLERemoteCharacteristic *pChr = pSvc->getCharacteristic(characteristicUUID);
    return ((pChr != nullptr) && pChr->canWrite() && pChr->writeValue(data, length, true));
  }

  return false;
}

bool CanonEOS::write_prefix(NimBLEClient *pClient,
                            const char *serviceUUID,
                            const char *characteristicUUID,
                            uint8_t prefix,
                            uint8_t *data,
                            size_t length) {
  uint8_t buffer[length + 1] = {0};
  buffer[0] = prefix;
  memcpy(&buffer[1], data, length);
  return write_value(pClient, serviceUUID, characteristicUUID, buffer, length + 1);
}

/**
 * Connect to a Canon EOS.
 *
 * The EOS uses the 'just works' BLE bonding to pair, all bond management is
 * handled by the underlying NimBLE and ESP32 libraries.
 */
bool CanonEOS::_connect(void) {
  if (NimBLEDevice::isBonded(m_Address)) {
    // Already bonded? Assume pair acceptance!
    m_PairResult = CANON_EOS_PAIR_ACCEPT;
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

  NimBLERemoteService *pSvc = m_Client->getService(CANON_EOS_SVC_IDEN_UUID);
  if (pSvc) {
    NimBLERemoteCharacteristic *pChr = pSvc->getCharacteristic(CANON_EOS_CHR_NAME_UUID);
    if ((pChr != nullptr) && pChr->canIndicate()) {
      ESP_LOGI(LOG_TAG, "Subscribed for pairing indication");
      pChr->subscribe(false,
                      [this](BLERemoteCharacteristic *pChr, uint8_t *pData, size_t length,
                             bool isNotify) { this->pairCallback(pChr, pData, length, isNotify); });
    }
  }

  ESP_LOGI(LOG_TAG, "Identifying 1!");
  const char *name = Device::getStringID();
  if (!write_prefix(m_Client, CANON_EOS_SVC_IDEN_UUID, CANON_EOS_CHR_NAME_UUID, 0x01,
                    (uint8_t *)name, strlen(name)))
    return false;

  m_Progress = 30;

  ESP_LOGI(LOG_TAG, "Identifying 2!");
  if (!write_prefix(m_Client, CANON_EOS_SVC_IDEN_UUID, CANON_EOS_CHR_IDEN_UUID, 0x03, m_Uuid.uint8,
                    UUID128_LEN))
    return false;

  m_Progress = 40;

  ESP_LOGI(LOG_TAG, "Identifying 3!");
  if (!write_prefix(m_Client, CANON_EOS_SVC_IDEN_UUID, CANON_EOS_CHR_IDEN_UUID, 0x04,
                    (uint8_t *)name, strlen(name)))
    return false;

  m_Progress = 50;

  ESP_LOGI(LOG_TAG, "Identifying 4!");

  uint8_t x = 0x02;
  if (!write_prefix(m_Client, CANON_EOS_SVC_IDEN_UUID, CANON_EOS_CHR_IDEN_UUID, 0x05, &x, 1))
    return false;

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

  if (m_PairResult != CANON_EOS_PAIR_ACCEPT) {
    bool deleted = NimBLEDevice::deleteBond(m_Address);
    ESP_LOGW(LOG_TAG, "Rejected, delete pairing: %d", deleted);
    return false;
  }

  ESP_LOGI(LOG_TAG, "Paired!");

  /* write to 0xf104 */
  x = 0x01;
  if (!write_value(m_Client, CANON_EOS_SVC_IDEN_UUID, CANON_EOS_CHR_IDEN_UUID, &x, 1))
    return false;

  m_Progress = 90;

  ESP_LOGI(LOG_TAG, "Switching mode!");

  /* write to 0xf307 */
  if (!write_value(m_Client, CANON_EOS_SVC_MODE_UUID, CANON_EOS_CHR_MODE_UUID,
                   &CANON_EOS_MODE_SHOOT, sizeof(CANON_EOS_MODE_SHOOT)))
    return false;

  ESP_LOGI(LOG_TAG, "Done!");
  m_Progress = 100;

  return true;
}

void CanonEOS::shutterPress(void) {
  uint8_t x[2] = {0x00, 0x01};
  write_value(m_Client, CANON_EOS_SVC_SHUTTER_UUID, CANON_EOS_CHR_SHUTTER_UUID, &x[0], 2);
}

void CanonEOS::shutterRelease(void) {
  uint8_t x[2] = {0x00, 0x02};
  write_value(m_Client, CANON_EOS_SVC_SHUTTER_UUID, CANON_EOS_CHR_SHUTTER_UUID, &x[0], 2);
}

void CanonEOS::focusPress(void) {
  // do nothing
  return;
}

void CanonEOS::focusRelease(void) {
  // do nothing
  return;
}

void CanonEOS::updateGeoData(const gps_t &gps, const timesync_t &timesync) {
  // do nothing
  return;
}

void CanonEOS::_disconnect(void) {
  m_Client->disconnect();
  m_Connected = false;
}

size_t CanonEOS::getSerialisedBytes(void) const {
  return sizeof(eos_t);
}

bool CanonEOS::serialise(void *buffer, size_t bytes) const {
  if (bytes != sizeof(eos_t)) {
    return false;
  }
  eos_t *x = static_cast<eos_t *>(buffer);
  strncpy(x->name, m_Name.c_str(), MAX_NAME);
  x->address = (uint64_t)m_Address;
  x->type = m_Address.getType();
  memcpy(&x->uuid, &m_Uuid, sizeof(Device::uuid128_t));

  return true;
}

}  // namespace Furble
