#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "Furble.h"

namespace Furble {

static volatile uint8_t pair_result = 0x00;

CanonEOS::CanonEOS(const void *data, size_t len) {
  if (len != sizeof(eos_t))
    throw;

  const eos_t *eos = (eos_t *)data;
  m_Name = std::string(eos->name);
  m_Address = NimBLEAddress(eos->address, eos->type);
  memcpy(&m_Uuid, &eos->uuid, sizeof(uuid128_t));
}

CanonEOS::CanonEOS(NimBLEAdvertisedDevice *pDevice) {
  m_Name = pDevice->getName();
  m_Address = pDevice->getAddress();
  Serial.println("Name = " + String(m_Name.c_str()));
  Serial.println("Address = " + String(m_Address.toString().c_str()));
  Device::getUUID128(&m_Uuid);
}

CanonEOS::~CanonEOS(void) {
  NimBLEDevice::deleteClient(m_Client);
  m_Client = nullptr;
}

// Handle pairing notification
static void pairResultCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                               uint8_t *pData,
                               size_t length,
                               bool isNotify) {
  if (!isNotify && (length > 0)) {
    Serial.printf("Got pairing callback: 0x%02x\n", pData[0]);
    pair_result = pData[0];
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
bool CanonEOS::connect(NimBLEClient *pClient, ezProgressBar &progress_bar) {
  m_Client = pClient;

  if (NimBLEDevice::isBonded(m_Address)) {
    // Already bonded? Assume pair acceptance!
    pair_result = CANON_EOS_PAIR_ACCEPT;
  } else {
    pair_result = 0x00;
  }

  Serial.println("Connecting");
  if (!m_Client->connect(m_Address)) {
    Serial.println("Connection failed!!!");
    return false;
  }

  Serial.println("Connected");
  progress_bar.value(10.0f);

  Serial.println("Securing");
  if (!m_Client->secureConnection()) {
    return false;
  }
  Serial.println("Secured!");
  progress_bar.value(20.0f);

  NimBLERemoteService *pSvc = pClient->getService(CANON_EOS_SVC_IDEN_UUID);
  if (pSvc) {
    NimBLERemoteCharacteristic *pChr = pSvc->getCharacteristic(CANON_EOS_CHR_NAME_UUID);
    if ((pChr != nullptr) && pChr->canIndicate()) {
      Serial.println("Subscribed for pairing indication");
      pChr->subscribe(false, pairResultCallback);
    }
  }

  Serial.println("Identifying 1!");
  if (!write_prefix(m_Client, CANON_EOS_SVC_IDEN_UUID, CANON_EOS_CHR_NAME_UUID, 0x01,
                    (uint8_t *)FURBLE_STR, strlen(FURBLE_STR)))
    return false;

  progress_bar.value(30.0f);

  Serial.println("Identifying 2!");
  if (!write_prefix(m_Client, CANON_EOS_SVC_IDEN_UUID, CANON_EOS_CHR_IDEN_UUID, 0x03, m_Uuid.uint8,
                    UUID128_LEN))
    return false;

  progress_bar.value(40.0f);

  Serial.println("Identifying 3!");
  if (!write_prefix(m_Client, CANON_EOS_SVC_IDEN_UUID, CANON_EOS_CHR_IDEN_UUID, 0x04,
                    (uint8_t *)FURBLE_STR, strlen(FURBLE_STR)))
    return false;

  progress_bar.value(50.0f);

  Serial.println("Identifying 4!");

  uint8_t x = 0x02;
  if (!write_prefix(m_Client, CANON_EOS_SVC_IDEN_UUID, CANON_EOS_CHR_IDEN_UUID, 0x05, &x, 1))
    return false;

  progress_bar.value(60.0f);

  Serial.println("Identifying 5!");

  /* write to 0xf204 */
  x = 0x0a;
  if (!write_value(m_Client, CANON_EOS_SVC_UNK0_UUID, CANON_EOS_CHR_UNK0_UUID, &x, 1))
    return false;

  // Give the user 60s to confirm/deny pairing
  Serial.println("Waiting for user to confirm/deny pairing.");
  for (unsigned int i = 0; i < 60; i++) {
    float progress = 70.0f + (float(i) / 6.0f);
    progress_bar.value(progress);
    if (pair_result != 0x00) {
      break;
    }
    delay(1000);
  }

  if (pair_result != CANON_EOS_PAIR_ACCEPT) {
    bool deleted = NimBLEDevice::deleteBond(m_Address);
    Serial.printf("Rejected, delete pairing: %d\n", deleted);
    return false;
  }

  /* write to 0xf104 */
  x = 0x01;
  if (!write_value(m_Client, CANON_EOS_SVC_IDEN_UUID, CANON_EOS_CHR_IDEN_UUID, &x, 1))
    return false;

  progress_bar.value(80.0f);

  Serial.println("Identifying 6!");

  /* write to 0xf307 */
  x = 0x03;
  if (!write_value(m_Client, CANON_EOS_SVC_UNK1_UUID, CANON_EOS_CHR_UNK1_UUID, &x, 1))
    return false;

  Serial.println("Paired!");
  progress_bar.value(100.0f);

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

void CanonEOS::updateGeoData(gps_t &gps, timesync_t &timesync) {
  // do nothing
  return;
}

void CanonEOS::disconnect(void) {
  m_Client->disconnect();
}

size_t CanonEOS::getSerialisedBytes(void) {
  return sizeof(eos_t);
}

bool CanonEOS::serialise(void *buffer, size_t bytes) {
  if (bytes != sizeof(eos_t)) {
    return false;
  }
  eos_t *x = (eos_t *)buffer;
  strncpy(x->name, m_Name.c_str(), MAX_NAME);
  x->address = (uint64_t)m_Address;
  x->type = m_Address.getType();
  memcpy(&x->uuid, &m_Uuid, sizeof(uuid128_t));

  return true;
}

}  // namespace Furble
