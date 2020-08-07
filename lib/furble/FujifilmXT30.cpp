#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteService.h>
#include <NimBLERemoteCharacteristic.h>

#include "Furble.h"

#define MAX_NAME 64

typedef struct _xt30_t {
  char name[MAX_NAME];    /** Human readable device name. */
  uint64_t address; /** Device MAC address. */
  uint8_t type;     /** Address type. */
  uint8_t token[XT30_TOKEN_LEN]; /** Pairing token. */
} xt30_t;

static const char *FUJI_XT30_SVC_PAIR_UUID = "91f1de68-dff6-466e-8b65-ff13b0f16fb8";
static const char *FUJI_XT30_CHR_PAIR_UUID = "aba356eb-9633-4e60-b73f-f52516dbd671";
static const char *FUJI_XT30_CHR_IDEN_UUID = "85b9163e-62d1-49ff-a6f5-054b4630d4a1";

// Currently unused
//static const char *FUJI_XT30_SVC_READ_UUID = "4e941240-d01d-46b9-a5ea-67636806830b";
//static const char *FUJI_XT30_CHR_READ_UUID = "bf6dc9cf-3606-4ec9-a4c8-d77576e93ea4";

static const char *FUJI_XT30_SVC_CONF_UUID = "4c0020fe-f3b6-40de-acc9-77d129067b14";
static const char *FUJI_XT30_CHR_IND1_UUID = "a68e3f66-0fcc-4395-8d4c-aa980b5877fa";
static const char *FUJI_XT30_CHR_IND2_UUID = "bd17ba04-b76b-4892-a545-b73ba1f74dae";
static const char *FUJI_XT30_CHR_NOT1_UUID = "f9150137-5d40-4801-a8dc-f7fc5b01da50";
static const char *FUJI_XT30_CHR_NOT2_UUID = "ad06c7b7-f41a-46f4-a29a-712055319122";
static const char *FUJI_XT30_CHR_NOT3_UUID = "049ec406-ef75-4205-a390-08fe209c51f0";

static const char *FUJI_XT30_SVC_SHUTTER_UUID = "6514eb81-4e8f-458d-aa2a-e691336cdfac";
static const char *FUJI_XT30_CHR_SHUTTER_UUID = "7fcf49c6-4ff0-4777-a03d-1a79166af7a8";

static const uint8_t FUJI_XT30_SHUTTER_CMD[2] = {0x01, 0x00};
static const uint8_t FUJI_XT30_SHUTTER_PRESS[2] = {0x02, 0x00};
static const uint8_t FUJI_XT30_SHUTTER_RELEASE[2] = {0x00, 0x00};

namespace Furble {

static void print_token(const uint8_t *token) {
  Serial.println("Token = " +
                 String(token[0], HEX) +
                 String(token[1], HEX) +
                 String(token[2], HEX) +
                 String(token[3], HEX));
}

FujifilmXT30::FujifilmXT30(const void *data, size_t len) {
  if (len != sizeof(xt30_t)) throw;

  const xt30_t *xt30 = (xt30_t *)data;
  m_Name = std::string(xt30->name);
  m_Address = NimBLEAddress(xt30->address, xt30->type);
  memcpy(m_Token, xt30->token, XT30_TOKEN_LEN);
}

FujifilmXT30::FujifilmXT30(NimBLEAdvertisedDevice *pDevice) {
  const char *data = pDevice->getManufacturerData().data();
  m_Name = pDevice->getName();
  m_Address = pDevice->getAddress();
  m_Token[0] = data[3];
  m_Token[1] = data[4];
  m_Token[2] = data[5];
  m_Token[3] = data[6];
  Serial.println("Name = " + String(m_Name.c_str()));
  Serial.println("Address = " + String(m_Address.toString().c_str()));
  print_token(m_Token);
}

FujifilmXT30::~FujifilmXT30(void)
{
  NimBLEDevice::deleteClient(m_Client);
  m_Client = nullptr;
}

const char *FujifilmXT30::getName(void) {
  return m_Name.c_str();
}

bool FujifilmXT30::connect(NimBLEClient *pClient,
                                 ezProgressBar &progress_bar)
{
  m_Client = pClient;

  progress_bar.value(10.0f);

  NimBLERemoteService *pSvc = nullptr;
  NimBLERemoteCharacteristic *pChr = nullptr;

  Serial.println("Connecting");
  if (!m_Client->connect(m_Address))
    return false;

  Serial.println("Connected");
  progress_bar.value(20.0f);
  pSvc = m_Client->getService(FUJI_XT30_SVC_PAIR_UUID);
  if (pSvc == nullptr) return false;

  Serial.println("Pairing");
  pChr = pSvc->getCharacteristic(FUJI_XT30_CHR_PAIR_UUID);
  if (pChr == nullptr) return false;

  if (!pChr->canWrite()) return false;
  print_token(m_Token);
  if (!pChr->writeValue(&m_Token[0], XT30_TOKEN_LEN))
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

  return true;
}

void FujifilmXT30::shutterPress(void)
{
  NimBLERemoteService *pSvc = m_Client->getService(FUJI_XT30_SVC_SHUTTER_UUID);
  NimBLERemoteCharacteristic *pChr = pSvc->getCharacteristic(FUJI_XT30_CHR_SHUTTER_UUID);
  pChr->writeValue(&FUJI_XT30_SHUTTER_CMD[0], sizeof(FUJI_XT30_SHUTTER_CMD), true);
  pChr->writeValue(&FUJI_XT30_SHUTTER_PRESS[0], sizeof(FUJI_XT30_SHUTTER_PRESS), true);
}

void FujifilmXT30::shutterRelease(void)
{
  NimBLERemoteService *pSvc = m_Client->getService(FUJI_XT30_SVC_SHUTTER_UUID);
  NimBLERemoteCharacteristic *pChr = pSvc->getCharacteristic(FUJI_XT30_CHR_SHUTTER_UUID);
  pChr->writeValue(&FUJI_XT30_SHUTTER_CMD[0], sizeof(FUJI_XT30_SHUTTER_CMD), true);
  pChr->writeValue(&FUJI_XT30_SHUTTER_RELEASE[0], sizeof(FUJI_XT30_SHUTTER_RELEASE), true);
}

void FujifilmXT30::print(void)
{
  Serial.print("Name: ");
  Serial.println(m_Name.c_str());
  Serial.print("Address: ");
  Serial.println(m_Address.toString().c_str());
  Serial.print("Type: ");
  Serial.println(m_Address.getType());
}

void FujifilmXT30::disconnect(void)
{
}

device_type_t FujifilmXT30::getDeviceType(void) {
  return FURBLE_FUJIFILM_XT30;
}

size_t FujifilmXT30::getSerialisedBytes(void) {
  return sizeof(xt30_t);
}

bool FujifilmXT30::serialise(void *buffer, size_t bytes) {
  if (bytes != sizeof(xt30_t)) {
    return false;
  }
  xt30_t *x = (xt30_t *)buffer;
  strncpy(x->name, m_Name.c_str(), 64);
  x->address = (uint64_t)m_Address;
  x->type = m_Address.getType();
  memcpy(x->token, m_Token, XT30_TOKEN_LEN);

  return true;
}

}
