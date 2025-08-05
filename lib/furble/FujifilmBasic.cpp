#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "Device.h"
#include "FujifilmBasic.h"

namespace Furble {

const NimBLEUUID FujifilmBasic::PRI1_SVC_UUID {0xaf854c2e, 0xb214, 0x458e, 0x97e2912c4ecf2cb8};
const NimBLEUUID FujifilmBasic::PRI2_SVC_UUID {0x117c4142, 0xedd4, 0x4c77, 0x8696dd18eebb770a};

void FujifilmBasic::print_token(const std::array<uint8_t, TOKEN_LEN> &token) {
  ESP_LOGI(LOG_TAG, "Token = %02x%02x%02x%02x", token[0], token[1], token[2], token[3]);
}

/**
 * Determine if the advertised BLE device is a Fujifilm basic.
 */
bool FujifilmBasic::matches(const NimBLEAdvertisedDevice *pDevice) {
  if (Fujifilm::matches(pDevice) && pDevice->getManufacturerData().length() == ADV_LEN) {
    return pDevice->isAdvertisingService(PRI1_SVC_UUID)
           || pDevice->isAdvertisingService(PRI2_SVC_UUID);
  }

  return false;
}

FujifilmBasic::FujifilmBasic(const void *data, size_t len)
    : Fujifilm(Type::FUJIFILM_BASIC, data, len) {
  if (len != sizeof(nvs_t))
    abort();

  const nvs_t *fujifilm = static_cast<const nvs_t *>(data);
  m_Name = std::string(fujifilm->name);
  m_Address = NimBLEAddress(fujifilm->address, fujifilm->type);
  memcpy(m_Token.data(), fujifilm->token, TOKEN_LEN);
}

FujifilmBasic::FujifilmBasic(const NimBLEAdvertisedDevice *pDevice)
    : Fujifilm(Type::FUJIFILM_BASIC, pDevice) {
  const adv_basic_t adv = pDevice->getManufacturerData<adv_basic_t>();
  m_Name = pDevice->getName();
  m_Address = pDevice->getAddress();
  m_Token = {adv.token[0], adv.token[1], adv.token[2], adv.token[3]};
  ESP_LOGI(LOG_TAG, "Name = %s", m_Name.c_str());
  ESP_LOGI(LOG_TAG, "Address = %s", m_Address.toString().c_str());
  print_token(m_Token);
}

/**
 * Connect to a Fujifilm basic camera.
 *
 * During initial pairing the advertisement includes a pairing token, this token
 * is what we use to identify ourselves upfront and during subsequent
 * re-pairing.
 */
bool FujifilmBasic::_connect(void) {
  m_Progress = 10;

  NimBLERemoteService *pSvc = nullptr;
  NimBLERemoteCharacteristic *pChr = nullptr;

  ESP_LOGI(LOG_TAG, "Connecting");
  if (!m_Client->connect(m_Address))
    return false;

  ESP_LOGI(LOG_TAG, "Connected");
  m_Progress = 20;
  pSvc = m_Client->getService(SVC_PAIR_UUID);
  if (pSvc == nullptr)
    return false;

  ESP_LOGI(LOG_TAG, "Pairing");
  pChr = pSvc->getCharacteristic(CHR_PAIR_UUID);
  if (pChr == nullptr)
    return false;

  if (!pChr->canWrite())
    return false;
  print_token(m_Token);
  if (!pChr->writeValue(m_Token.data(), m_Token.size(), true))
    return false;
  ESP_LOGI(LOG_TAG, "Paired!");
  m_Progress = 30;

  ESP_LOGI(LOG_TAG, "Identifying");
  pChr = pSvc->getCharacteristic(CHR_IDEN_UUID);
  if (!pChr->canWrite())
    return false;
  const auto name = Device::getStringID();
  if (!pChr->writeValue(name.c_str(), name.length(), true))
    return false;
  ESP_LOGI(LOG_TAG, "Identified!");
  m_Progress = 40;

  // indications
  ESP_LOGI(LOG_TAG, "Configuring");
  if (!this->subscribe(SVC_CONF_UUID, CHR_IND1_UUID, false)) {
    return false;
  }

  if (!this->subscribe(SVC_CONF_UUID, CHR_IND2_UUID, false)) {
    return false;
  }
  ESP_LOGI(LOG_TAG, "Configured");
  m_Progress = 50;

#if 0
  // wait for up to (10*500)ms callback
  for (unsigned int i = 0; i < 10; i++) {
    if (m_Configured) {
      break;
    }
    m_Progress = m_Progress.load() + 1;
    vTaskDelay(pdMS_TO_TICKS(500));
  }
#endif

  m_Progress = 60;
  // notifications
  if (!this->subscribe(SVC_CONF_UUID, CHR_NOT1_UUID, true)) {
    return false;
  }
  m_Progress = 70;

  if (!this->subscribe(SVC_CONF_UUID, CHR_NOT2_UUID, true)) {
    return false;
  }
  m_Progress = 80;

  if (!this->subscribe(SVC_CONF_UUID, CHR_IND3_UUID, false)) {
    return false;
  }

  ESP_LOGI(LOG_TAG, "Getting shutter service");
  pSvc = m_Client->getService(SVC_SHUTTER_UUID);
  if (pSvc == nullptr) {
    ESP_LOGI(LOG_TAG, "Failed to get shutter service");
  }

  m_Progress = 90;

  ESP_LOGI(LOG_TAG, "Getting shutter characteristic");
  m_Shutter = pSvc->getCharacteristic(CHR_SHUTTER_UUID);
  if (m_Shutter == nullptr) {
    ESP_LOGI(LOG_TAG, "Failed to get shutter characteristic");
  }

  m_Progress = 100;

  ESP_LOGI(LOG_TAG, "Connected");

  return true;
}

size_t FujifilmBasic::getSerialisedBytes(void) const {
  return sizeof(nvs_t);
}

bool FujifilmBasic::serialise(void *buffer, size_t bytes) const {
  if (bytes != sizeof(nvs_t)) {
    return false;
  }
  nvs_t *x = static_cast<nvs_t *>(buffer);
  strncpy(x->name, m_Name.c_str(), MAX_NAME);
  x->address = (uint64_t)m_Address;
  x->type = m_Address.getType();
  memcpy(x->token, m_Token.data(), TOKEN_LEN);

  return true;
}

}  // namespace Furble
