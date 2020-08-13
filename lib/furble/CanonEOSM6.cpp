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

namespace Furble {

CanonEOSM6::CanonEOSM6(const void *data, size_t len) {
  if (len != sizeof(eosm6_t)) throw;

  const eosm6_t *eosm6 = (eosm6_t *)data;
  m_Name = std::string(eosm6->name);
  m_Address = NimBLEAddress(eosm6->address, eosm6->type);
  memcpy(&m_Uuid, &eosm6->uuid, sizeof(uuid128_t));
}

CanonEOSM6::CanonEOSM6(NimBLEAdvertisedDevice *pDevice) {
  //const char *data = pDevice->getManufacturerData().data();
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

bool CanonEOSM6::matches(NimBLEAdvertisedDevice *pDevice) {
  return false;
}


const char *CanonEOSM6::getName(void) {
  return m_Name.c_str();
}

bool CanonEOSM6::connect(NimBLEClient *pClient,
                         ezProgressBar &progress_bar)
{
  m_Client = pClient;

  progress_bar.value(10.0f);

  //NimBLERemoteService *pSvc = nullptr;
  //NimBLERemoteCharacteristic *pChr = nullptr;
  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID | BLE_SM_PAIR_KEY_DIST_SIGN);
  NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID | BLE_SM_PAIR_KEY_DIST_SIGN);

  Serial.println("Connecting");
  if (!m_Client->connect(m_Address)) {
    Serial.println("Connection failed!!!");
    return false;
  }

#if 0
  Serial.println("Connected");
  progress_bar.value(20.0f);
  pSvc = m_Client->getService(FUJI_XT30_SVC_PAIR_UUID);
  if (pSvc == nullptr) return false;

  Serial.println("Pairing");
  pChr = pSvc->getCharacteristic(FUJI_XT30_CHR_PAIR_UUID);
  if (pChr == nullptr) return false;

  if (!pChr->canWrite()) return false;
  print_token(m_Token);
  if (!pChr->writeValue(m_Token, XT30_TOKEN_LEN))
    return false;
  Serial.println("Paired!");
  progress_bar.value(30.0f);

  Serial.println("Identifying");
  pChr = pSvc->getCharacteristic(FUJI_XT30_CHR_IDEN_UUID);
  if (!pChr->canWrite()) return false;
  if (!pChr->writeValue(FURBLE_STR)) return false;
  Serial.println("Identified!");
  progress_bar.value(40.0f);

  Serial.println("Configuring");
  pSvc = m_Client->getService(FUJI_XT30_SVC_CONF_UUID);
  // indications
  pSvc->getCharacteristic(FUJI_XT30_CHR_IND1_UUID)->subscribe(false);
  progress_bar.value(50.0f);
  pSvc->getCharacteristic(FUJI_XT30_CHR_IND2_UUID)->subscribe(false);
  progress_bar.value(60.0f);
  // notifications
  pSvc->getCharacteristic(FUJI_XT30_CHR_NOT1_UUID)->subscribe(true);
  progress_bar.value(70.0f);
  pSvc->getCharacteristic(FUJI_XT30_CHR_NOT2_UUID)->subscribe(true);
  progress_bar.value(80.0f);
  pSvc->getCharacteristic(FUJI_XT30_CHR_NOT3_UUID)->subscribe(true);
  progress_bar.value(90.0f);

  Serial.println("Configured");

  progress_bar.value(100.0f);
#endif

  return true;
}

void CanonEOSM6::shutterPress(void)
{
}

void CanonEOSM6::shutterRelease(void)
{
}

void CanonEOSM6::disconnect(void)
{
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
