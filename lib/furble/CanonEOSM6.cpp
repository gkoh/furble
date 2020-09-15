#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "Furble.h"

typedef struct _eosm6_t {
  char name[MAX_NAME];    /** Human readable device name. */
  uint64_t address; /** Device MAC address. */
  uint8_t type;     /** Address type. */
  uuid128_t uuid; /** Our UUID. */
} eosm6_t;

static const char *CANON_EOSM6_SVC_IDEN_UUID = "00010000-0000-1000-0000-d8492fffa821";
/** 0xf108 */
static const char *CANON_EOSM6_CHR_NAME_UUID = "00010006-0000-1000-0000-d8492fffa821";
/** 0xf104 */
static const char *CANON_EOSM6_CHR_IDEN_UUID = "0001000a-0000-1000-0000-d8492fffa821";

static const char *CANON_EOSM6_SVC_UNK0_UUID = "00020000-0000-1000-0000-d8492fffa821";
/** 0xf204 */
static const char *CANON_EOSM6_CHR_UNK0_UUID = "00020002-0000-1000-0000-d8492fffa821";

static const char *CANON_EOSM6_SVC_UNK1_UUID = "00030000-0000-1000-0000-d8492fffa821";
/** 0xf307 */
static const char *CANON_EOSM6_CHR_UNK1_UUID = "00030010-0000-1000-0000-d8492fffa821";

static const char *CANON_EOSM6_SVC_SHUTTER_UUID = "00030000-0000-1000-0000-d8492fffa821";
/** 0xf311 */
static const char *CANON_EOSM6_CHR_SHUTTER_UUID = "00030030-0000-1000-0000-d8492fffa821";

namespace Furble {

CanonEOSM6::CanonEOSM6(const void *data, size_t len) {
  if (len != sizeof(eosm6_t)) throw;

  const eosm6_t *eosm6 = (eosm6_t *)data;
  m_Name = std::string(eosm6->name);
  m_Address = NimBLEAddress(eosm6->address, eosm6->type);
  memcpy(&m_Uuid, &eosm6->uuid, sizeof(uuid128_t));
}

CanonEOSM6::CanonEOSM6(NimBLEAdvertisedDevice *pDevice) {
  m_Name = pDevice->getName();
  m_Address = pDevice->getAddress();
  Serial.println("Name = " + String(m_Name.c_str()));
  Serial.println("Address = " + String(m_Address.toString().c_str()));
  Device::getUUID128(&m_Uuid);
}

CanonEOSM6::~CanonEOSM6(void)
{
  NimBLEDevice::deleteClient(m_Client);
  m_Client = nullptr;
}

const size_t CANON_EOS_M6_ADV_DATA_LEN = 21;
const uint8_t CANON_EOS_M6_ID_0 = 0xa9;
const uint8_t CANON_EOS_M6_ID_1 = 0x01;
const uint8_t CANON_EOS_M6_XX_2 = 0x01;
const uint8_t CANON_EOS_M6_XX_3 = 0xc5;
const uint8_t CANON_EOS_M6_XX_4 = 0x32;

bool CanonEOSM6::matches(NimBLEAdvertisedDevice *pDevice) {
  if (pDevice->haveManufacturerData() &&
      pDevice->getManufacturerData().length() == CANON_EOS_M6_ADV_DATA_LEN) {
    const char *data = pDevice->getManufacturerData().data();
    if (data[0] == CANON_EOS_M6_ID_0 &&
        data[1] == CANON_EOS_M6_ID_1 &&
        data[2] == CANON_EOS_M6_XX_2 &&
        data[3] == CANON_EOS_M6_XX_3 &&
        data[4] == CANON_EOS_M6_XX_4) {
      // All remaining bits should be zero.
      uint8_t zero = 0;
      for (size_t i = 5; i < CANON_EOS_M6_ADV_DATA_LEN; i++) {
        zero |= data[i];
      }
      return (zero == 0);
    }
  }
  return false;
}

static bool write_value(NimBLEClient *pClient,
                        const char *serviceUUID,
                        const char *characteristicUUID,
                        uint8_t *data,
                        size_t length) {
  NimBLERemoteService *pSvc = pClient->getService(serviceUUID);
  if (pSvc) {
    NimBLERemoteCharacteristic *pChr = pSvc->getCharacteristic(characteristicUUID);
    return ((pChr != nullptr) &&
            pChr->canWrite() &&
            pChr->writeValue(data, length, true));
  }

  return false;
}

static bool write_prefix(NimBLEClient *pClient,
                         const char *serviceUUID,
                         const char *characteristicUUID,
                         uint8_t prefix,
                         uint8_t *data,
                         size_t length) {

  uint8_t buffer[length+1] = { 0 };
  buffer[0] = prefix;
  memcpy(&buffer[1], data, length);
  return write_value(pClient, serviceUUID, characteristicUUID, buffer, length+1);
}

/**
 * Connect to a Canon EOS M6.
 *
 * The EOS M6 uses the 'just works' BLE bonding to pair, all bond management is
 * handled by the underlying NimBLE and ESP32 libraries.
 */
bool CanonEOSM6::connect(NimBLEClient *pClient,
                         ezProgressBar &progress_bar)
{
  m_Client = pClient;

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

  Serial.println("Identifying 1!");
  if (!write_prefix(m_Client,
                    CANON_EOSM6_SVC_IDEN_UUID,
                    CANON_EOSM6_CHR_NAME_UUID,
                    0x01, (uint8_t *)FURBLE_STR, strlen(FURBLE_STR)))
    return false;

  progress_bar.value(30.0f);

  Serial.println("Identifying 2!");
  if (!write_prefix(m_Client,
                    CANON_EOSM6_SVC_IDEN_UUID,
                    CANON_EOSM6_CHR_IDEN_UUID,
                    0x03, m_Uuid.uint8, UUID128_LEN))
    return false;

  progress_bar.value(40.0f);

  Serial.println("Identifying 3!");
  if (!write_prefix(m_Client,
                    CANON_EOSM6_SVC_IDEN_UUID,
                    CANON_EOSM6_CHR_IDEN_UUID,
                    0x04, (uint8_t *)FURBLE_STR, strlen(FURBLE_STR)))
    return false;

  progress_bar.value(50.0f);

  Serial.println("Identifying 4!");

  uint8_t x = 0x02;
  if (!write_prefix(m_Client,
                    CANON_EOSM6_SVC_IDEN_UUID,
                    CANON_EOSM6_CHR_IDEN_UUID,
                    0x05, &x, 1))
    return false;

  progress_bar.value(60.0f);

  Serial.println("Identifying 5!");

  /* write to 0xf204 */
  x = 0x0a;
  if (!write_value(m_Client,
                   CANON_EOSM6_SVC_UNK0_UUID,
                   CANON_EOSM6_CHR_UNK0_UUID,
                   &x, 1))
    return false;

  progress_bar.value(70.0f);

  /* write to 0xf104 */
  x = 0x01;
  if (!write_value(m_Client,
                   CANON_EOSM6_SVC_IDEN_UUID,
                   CANON_EOSM6_CHR_IDEN_UUID,
                   &x, 1))
    return false;

  progress_bar.value(80.0f);

  Serial.println("Identifying 6!");

  /* write to 0xf307 */
  x = 0x03;
  if (!write_value(m_Client,
                   CANON_EOSM6_SVC_UNK1_UUID,
                   CANON_EOSM6_CHR_UNK1_UUID,
                   &x, 1))
    return false;

  Serial.println("Paired!");
  progress_bar.value(100.0f);

  return true;
}

void CanonEOSM6::shutterPress(void) {
  uint8_t x[2] = { 0x00, 0x01 };
  write_value(m_Client,
              CANON_EOSM6_SVC_SHUTTER_UUID,
              CANON_EOSM6_CHR_SHUTTER_UUID,
              &x[0], 2);
}

void CanonEOSM6::shutterRelease(void) {
  uint8_t x[2] = { 0x00, 0x02 };
  write_value(m_Client,
              CANON_EOSM6_SVC_SHUTTER_UUID,
              CANON_EOSM6_CHR_SHUTTER_UUID,
              &x[0], 2);
}

void CanonEOSM6::disconnect(void) {
  m_Client->disconnect();
}

device_type_t CanonEOSM6::getDeviceType(void) {
  return FURBLE_CANON_EOS_M6;
}

size_t CanonEOSM6::getSerialisedBytes(void) {
  return sizeof(eosm6_t);
}

bool CanonEOSM6::serialise(void *buffer, size_t bytes) {
  if (bytes != sizeof(eosm6_t)) {
    return false;
  }
  eosm6_t *x = (eosm6_t *)buffer;
  strncpy(x->name, m_Name.c_str(), MAX_NAME);
  x->address = (uint64_t)m_Address;
  x->type = m_Address.getType();
  memcpy(&x->uuid, &m_Uuid, sizeof(uuid128_t));

  return true;
}

}
