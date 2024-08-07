#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "Device.h"
#include "Fujifilm.h"

typedef struct _fujifilm_t {
  char name[MAX_NAME];               /** Human readable device name. */
  uint64_t address;                  /** Device MAC address. */
  uint8_t type;                      /** Address type. */
  uint8_t token[FUJIFILM_TOKEN_LEN]; /** Pairing token. */
} fujifilm_t;

// 0x4001
static const NimBLEUUID FUJIFILM_SVC_PAIR_UUID = NimBLEUUID("91f1de68-dff6-466e-8b65-ff13b0f16fb8");
// 0x4042
static const NimBLEUUID FUJIFILM_CHR_PAIR_UUID = NimBLEUUID("aba356eb-9633-4e60-b73f-f52516dbd671");
// 0x4012
static const NimBLEUUID FUJIFILM_CHR_IDEN_UUID = NimBLEUUID("85b9163e-62d1-49ff-a6f5-054b4630d4a1");

// Currently unused
// static const char *FUJIFILM_SVC_READ_UUID =
// "4e941240-d01d-46b9-a5ea-67636806830b"; static const char
// *FUJIFILM_CHR_READ_UUID = "bf6dc9cf-3606-4ec9-a4c8-d77576e93ea4";

static const NimBLEUUID FUJIFILM_SVC_CONF_UUID = NimBLEUUID("4c0020fe-f3b6-40de-acc9-77d129067b14");
// 0x5013
static const NimBLEUUID FUJIFILM_CHR_IND1_UUID = NimBLEUUID("a68e3f66-0fcc-4395-8d4c-aa980b5877fa");
// 0x5023
static const NimBLEUUID FUJIFILM_CHR_IND2_UUID = NimBLEUUID("bd17ba04-b76b-4892-a545-b73ba1f74dae");
// 0x5033
static const NimBLEUUID FUJIFILM_CHR_NOT1_UUID = NimBLEUUID("f9150137-5d40-4801-a8dc-f7fc5b01da50");
// 0x5043
static const NimBLEUUID FUJIFILM_CHR_NOT2_UUID = NimBLEUUID("ad06c7b7-f41a-46f4-a29a-712055319122");
static const NimBLEUUID FUJIFILM_CHR_IND3_UUID = NimBLEUUID("049ec406-ef75-4205-a390-08fe209c51f0");

static const NimBLEUUID FUJIFILM_SVC_SHUTTER_UUID =
    NimBLEUUID("6514eb81-4e8f-458d-aa2a-e691336cdfac");
static const NimBLEUUID FUJIFILM_CHR_SHUTTER_UUID =
    NimBLEUUID("7fcf49c6-4ff0-4777-a03d-1a79166af7a8");

static const NimBLEUUID FUJIFILM_SVC_GEOTAG_UUID =
    NimBLEUUID("3b46ec2b-48ba-41fd-b1b8-ed860b60d22b");
static const NimBLEUUID FUJIFILM_CHR_GEOTAG_UUID =
    NimBLEUUID("0f36ec14-29e5-411a-a1b6-64ee8383f090");

static const NimBLEUUID FUJIFILM_GEOTAG_UPDATE = NimBLEUUID("ad06c7b7-f41a-46f4-a29a-712055319122");

static const uint8_t FUJIFILM_SHUTTER_CMD[2] = {0x01, 0x00};
static const uint8_t FUJIFILM_SHUTTER_PRESS[2] = {0x02, 0x00};
static const uint8_t FUJIFILM_SHUTTER_RELEASE[2] = {0x00, 0x00};
static const uint8_t FUJIFILM_SHUTTER_FOCUS[2] = {0x03, 0x00};

namespace Furble {

static void print_token(const uint8_t *token) {
  Serial.printf("Token = %02x%02x%02x%02x\r\n", token[0], token[1], token[2], token[3]);
}

void Fujifilm::notify(BLERemoteCharacteristic *pChr, uint8_t *pData, size_t length, bool isNotify) {
  Serial.printf("Got %s callback: %u bytes from %s\r\n", isNotify ? "notification" : "indication",
                length, pChr->getUUID().toString().c_str());
  if (length > 0) {
    for (int i = 0; i < length; i++) {
      Serial.printf("  [%d] 0x%02x\r\n", i, pData[i]);
    }
  }

  if (pChr->getUUID() == FUJIFILM_CHR_NOT1_UUID) {
    if ((length >= 2) && (pData[0] == 0x02) && (pData[1] == 0x00)) {
      m_Configured = true;
    }
  } else if (pChr->getUUID() == FUJIFILM_GEOTAG_UPDATE) {
    if ((length >= 2) && (pData[0] == 0x01) && (pData[1] == 0x00)) {
      m_GeoRequested = true;
    }
  } else {
    Serial.println("Unhandled notification handle.");
  }
}

Fujifilm::Fujifilm(const void *data, size_t len) {
  if (len != sizeof(fujifilm_t))
    throw;

  const fujifilm_t *fujifilm = static_cast<const fujifilm_t *>(data);
  m_Name = std::string(fujifilm->name);
  m_Address = NimBLEAddress(fujifilm->address, fujifilm->type);
  memcpy(m_Token, fujifilm->token, FUJIFILM_TOKEN_LEN);
}

Fujifilm::Fujifilm(NimBLEAdvertisedDevice *pDevice) {
  const char *data = pDevice->getManufacturerData().data();
  m_Name = pDevice->getName();
  m_Address = pDevice->getAddress();
  m_Token[0] = data[3];
  m_Token[1] = data[4];
  m_Token[2] = data[5];
  m_Token[3] = data[6];
  Serial.printf("Name = %s\r\n", m_Name.c_str());
  Serial.printf("Address = %s\r\n", m_Address.toString().c_str());
  print_token(m_Token);
}

Fujifilm::~Fujifilm(void) {
  NimBLEDevice::deleteClient(m_Client);
  m_Client = nullptr;
}

const size_t FUJIFILM_ADV_TOKEN_LEN = 7;
const uint8_t FUJIFILM_ID_0 = 0xd8;
const uint8_t FUJIFILM_ID_1 = 0x04;
const uint8_t FUJIFILM_TYPE_TOKEN = 0x02;

/**
 * Determine if the advertised BLE device is a Fujifilm X-T30.
 */
bool Fujifilm::matches(NimBLEAdvertisedDevice *pDevice) {
  if (pDevice->haveManufacturerData()
      && pDevice->getManufacturerData().length() == FUJIFILM_ADV_TOKEN_LEN) {
    const char *data = pDevice->getManufacturerData().data();
    if (data[0] == FUJIFILM_ID_0 && data[1] == FUJIFILM_ID_1 && data[2] == FUJIFILM_TYPE_TOKEN) {
      return true;
    }
  }
  return false;
}

/**
 * Connect to a Fujifilm.
 *
 * During initial pairing the advertisement includes a pairing token, this token
 * is what we use to identify ourselves upfront and during subsequent
 * re-pairing.
 */
bool Fujifilm::connect(progressFunc pFunc, void *pCtx) {
  using namespace std::placeholders;

  updateProgress(pFunc, pCtx, 10.0f);

  NimBLERemoteService *pSvc = nullptr;
  NimBLERemoteCharacteristic *pChr = nullptr;

  Serial.println("Connecting");
  if (!m_Client->connect(m_Address))
    return false;

  Serial.println("Connected");
  updateProgress(pFunc, pCtx, 20.0f);
  pSvc = m_Client->getService(FUJIFILM_SVC_PAIR_UUID);
  if (pSvc == nullptr)
    return false;

  Serial.println("Pairing");
  pChr = pSvc->getCharacteristic(FUJIFILM_CHR_PAIR_UUID);
  if (pChr == nullptr)
    return false;

  if (!pChr->canWrite())
    return false;
  print_token(m_Token);
  if (!pChr->writeValue(m_Token, FUJIFILM_TOKEN_LEN, true))
    return false;
  Serial.println("Paired!");
  updateProgress(pFunc, pCtx, 30.0f);

  Serial.println("Identifying");
  pChr = pSvc->getCharacteristic(FUJIFILM_CHR_IDEN_UUID);
  if (!pChr->canWrite())
    return false;
  if (!pChr->writeValue(Device::getStringID(), true))
    return false;
  Serial.println("Identified!");
  updateProgress(pFunc, pCtx, 40.0f);

  Serial.println("Configuring");
  pSvc = m_Client->getService(FUJIFILM_SVC_CONF_UUID);
  // indications
  pSvc->getCharacteristic(FUJIFILM_CHR_IND1_UUID)
      ->subscribe(
          false,
          [this](BLERemoteCharacteristic *pChr, uint8_t *pData, size_t length, bool isNotify) {
            this->notify(pChr, pData, length, isNotify);
          },
          true);
  updateProgress(pFunc, pCtx, 50.0f);

  pSvc->getCharacteristic(FUJIFILM_CHR_IND2_UUID)
      ->subscribe(
          false,
          [this](BLERemoteCharacteristic *pChr, uint8_t *pData, size_t length, bool isNotify) {
            this->notify(pChr, pData, length, isNotify);
          },
          true);

  // wait for up to 5000ms callback
  for (unsigned int i = 0; i < 5000; i += 100) {
    if (m_Configured) {
      break;
    }
    updateProgress(pFunc, pCtx, 50.0f + (((float)i / 5000.0f) * 10.0f));
    delay(100);
  }

  updateProgress(pFunc, pCtx, 60.0f);
  // notifications
  pSvc->getCharacteristic(FUJIFILM_CHR_NOT1_UUID)
      ->subscribe(
          true,
          [this](BLERemoteCharacteristic *pChr, uint8_t *pData, size_t length, bool isNotify) {
            this->notify(pChr, pData, length, isNotify);
          },
          true);

  updateProgress(pFunc, pCtx, 70.0f);
  pSvc->getCharacteristic(FUJIFILM_CHR_NOT2_UUID)
      ->subscribe(
          true,
          [this](BLERemoteCharacteristic *pChr, uint8_t *pData, size_t length, bool isNotify) {
            this->notify(pChr, pData, length, isNotify);
          },
          true);

  updateProgress(pFunc, pCtx, 80.0f);
  pSvc->getCharacteristic(FUJIFILM_CHR_IND3_UUID)
      ->subscribe(
          false,
          [this](BLERemoteCharacteristic *pChr, uint8_t *pData, size_t length, bool isNotify) {
            this->notify(pChr, pData, length, isNotify);
          },
          true);
  Serial.println("Configured");

  updateProgress(pFunc, pCtx, 100.0f);

  return true;
}

void Fujifilm::shutterPress(void) {
  NimBLERemoteService *pSvc = m_Client->getService(FUJIFILM_SVC_SHUTTER_UUID);
  NimBLERemoteCharacteristic *pChr = pSvc->getCharacteristic(FUJIFILM_CHR_SHUTTER_UUID);
  pChr->writeValue(&FUJIFILM_SHUTTER_CMD[0], sizeof(FUJIFILM_SHUTTER_CMD), true);
  pChr->writeValue(&FUJIFILM_SHUTTER_PRESS[0], sizeof(FUJIFILM_SHUTTER_PRESS), true);
}

void Fujifilm::shutterRelease(void) {
  NimBLERemoteService *pSvc = m_Client->getService(FUJIFILM_SVC_SHUTTER_UUID);
  NimBLERemoteCharacteristic *pChr = pSvc->getCharacteristic(FUJIFILM_CHR_SHUTTER_UUID);
  pChr->writeValue(&FUJIFILM_SHUTTER_CMD[0], sizeof(FUJIFILM_SHUTTER_CMD), true);
  pChr->writeValue(&FUJIFILM_SHUTTER_RELEASE[0], sizeof(FUJIFILM_SHUTTER_RELEASE), true);
}

void Fujifilm::focusPress(void) {
  NimBLERemoteService *pSvc = m_Client->getService(FUJIFILM_SVC_SHUTTER_UUID);
  NimBLERemoteCharacteristic *pChr = pSvc->getCharacteristic(FUJIFILM_CHR_SHUTTER_UUID);
  pChr->writeValue(&FUJIFILM_SHUTTER_CMD[0], sizeof(FUJIFILM_SHUTTER_CMD), true);
  pChr->writeValue(&FUJIFILM_SHUTTER_FOCUS[0], sizeof(FUJIFILM_SHUTTER_FOCUS), true);
}

void Fujifilm::focusRelease(void) {
  shutterRelease();
}

void Fujifilm::sendGeoData(gps_t &gps, timesync_t &timesync) {
  NimBLERemoteService *pSvc = m_Client->getService(FUJIFILM_SVC_GEOTAG_UUID);
  if (pSvc == nullptr) {
    return;
  }

  NimBLERemoteCharacteristic *pChr = pSvc->getCharacteristic(FUJIFILM_CHR_GEOTAG_UUID);
  if (pChr == nullptr) {
    return;
  }

  if (pChr->canWrite()) {
    geotag_t geotag = {.latitude = (int32_t)(gps.latitude * 10000000),
                       .longitude = (int32_t)(gps.longitude * 10000000),
                       .altitude = (int32_t)gps.altitude,
                       .pad = {0},
                       .gps_time = {
                           .year = (uint16_t)timesync.year,
                           .day = (uint8_t)timesync.day,
                           .month = (uint8_t)timesync.month,
                           .hour = (uint8_t)timesync.hour,
                           .minute = (uint8_t)timesync.minute,
                           .second = (uint8_t)timesync.second,
                       }};

    Serial.printf("Sending geotag data (%u bytes) to 0x%04x\r\n", sizeof(geotag),
                  pChr->getHandle());
    Serial.printf("  lat: %f, %d\r\n", gps.latitude, geotag.latitude);
    Serial.printf("  lon: %f, %d\r\n", gps.longitude, geotag.longitude);
    Serial.printf("  alt: %f, %d\r\n", gps.altitude, geotag.altitude);

    pChr->writeValue((uint8_t *)&geotag, sizeof(geotag), true);
  }
}

void Fujifilm::updateGeoData(gps_t &gps, timesync_t &timesync) {
  if (m_GeoRequested) {
    sendGeoData(gps, timesync);
    m_GeoRequested = false;
  }
}

void Fujifilm::print(void) {
  Serial.printf("Name: %s\r\n", m_Name.c_str());
  Serial.printf("Address: %s\r\n", m_Address.toString().c_str());
  Serial.printf("Type: %d\r\n", m_Address.getType());
}

void Fujifilm::disconnect(void) {
  m_Client->disconnect();
}

device_type_t Fujifilm::getDeviceType(void) {
  return FURBLE_FUJIFILM;
}

size_t Fujifilm::getSerialisedBytes(void) {
  return sizeof(fujifilm_t);
}

bool Fujifilm::serialise(void *buffer, size_t bytes) {
  if (bytes != sizeof(fujifilm_t)) {
    return false;
  }
  fujifilm_t *x = static_cast<fujifilm_t *>(buffer);
  strncpy(x->name, m_Name.c_str(), MAX_NAME);
  x->address = (uint64_t)m_Address;
  x->type = m_Address.getType();
  memcpy(x->token, m_Token, FUJIFILM_TOKEN_LEN);

  return true;
}

}  // namespace Furble
