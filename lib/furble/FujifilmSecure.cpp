#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "Device.h"
#include "FujifilmSecure.h"

namespace Furble {

const NimBLEUUID FujifilmSecure::SERVICE_UUID {0xa9d2b304, 0xe8d6, 0x4902, 0x8336352b772d7597};
const NimBLEUUID FujifilmSecure::PRI_SVC_UUID {0x731893f9, 0x744e, 0x4899, 0xb7e3174106ff2b82};

/**
 * Determine if the advertised BLE device is a Fujifilm secure camera.
 */
bool FujifilmSecure::matches(const NimBLEAdvertisedDevice *pDevice) {
  return (Fujifilm::matches(pDevice) && pDevice->isAdvertisingService(SERVICE_UUID));
}

FujifilmSecure::FujifilmSecure(const void *data, size_t len)
    : Fujifilm(Type::FUJIFILM_SECURE, data, len) {
  if (len != sizeof(nvs_t))
    abort();

  const nvs_t *fujifilm = static_cast<const nvs_t *>(data);
  m_Name = std::string(fujifilm->name);
  m_Address = NimBLEAddress(fujifilm->address, fujifilm->type);
}

FujifilmSecure::FujifilmSecure(const NimBLEAdvertisedDevice *pDevice)
    : Fujifilm(Type::FUJIFILM_SECURE, pDevice) {
  m_Name = pDevice->getName();
  m_Address = pDevice->getAddress();
  ESP_LOGI(LOG_TAG, "Name = %s", m_Name.c_str());
  ESP_LOGI(LOG_TAG, "Address = %s", m_Address.toString().c_str());
}

bool FujifilmSecure::subscribeNotification(const NimBLEUUID &svc, const NimBLEUUID &chr) {
  auto pSvc = m_Client->getService(svc);
  if (pSvc == nullptr) {
    return false;
  }

  auto pChr = pSvc->getCharacteristic(chr);
  if (pChr == nullptr) {
    return false;
  }

  return pChr->subscribe(
      true,
      [this](BLERemoteCharacteristic *pChr, uint8_t *pData, size_t length, bool isNotify) {
        ESP_LOGI(LOG_TAG, "Notification received on %s", pChr->getUUID().toString().c_str());
        if (length > 0) {
          ESP_LOGI(LOG_TAG, " %s", NimBLEUtils::dataToHexString(pData, length).c_str());
        }
      },
      true);
}

/**
 * Connect to a Fujifilm secure.
 */
bool FujifilmSecure::_connect(void) {
  m_Progress = 10;

  // NimBLERemoteCharacteristic *pChr = nullptr;

  ESP_LOGI(LOG_TAG, "Connecting");
  if (!m_Client->connect(m_Address))
    return false;

  ESP_LOGI(LOG_TAG, "Connected");
  m_Progress = 20;

  ESP_LOGI(LOG_TAG, "Securing");
  if (!m_Client->secureConnection()) {
    return false;
  }
  ESP_LOGI(LOG_TAG, "Secured!");
  m_Progress = 40;

  ESP_LOGI(LOG_TAG, "Requesting status");
  auto status = m_Client->getValue(PAIR_SVC_UUID, STATUS_CHR_UUID);
  if (status.size() == 4) {
    ESP_LOGI(LOG_TAG, "Status: %s",
             NimBLEUtils::dataToHexString(status.data(), status.size()).c_str());
    const auto ack = NimBLEAttValue({status[0], status[1], 0x02, status[3]});
    ESP_LOGI(LOG_TAG, "Responding status with %s",
             NimBLEUtils::dataToHexString(ack.data(), ack.size()).c_str());
    if (!m_Client->setValue(PAIR_SVC_UUID, STATUS_CHR_UUID, ack, true)) {
      ESP_LOGI(LOG_TAG, "Failed to write status response");
      return false;
    }
  } else {
    ESP_LOGI(LOG_TAG, "Failed to request status");
    return false;
  }
  m_Progress = 50;

  auto name = NimBLEAttValue(Device::getStringID());
  ESP_LOGI(LOG_TAG, "Identifying as %s", name.c_str());
  if (!m_Client->setValue(PAIR_SVC_UUID, IDENT_CHR_UUID, name, true)) {
    ESP_LOGI(LOG_TAG, "Failed to send identifier");
    return false;
  }
  ESP_LOGI(LOG_TAG, "Identified!");
  m_Progress = 60;

  ESP_LOGI(LOG_TAG, "Subscribing to notification 1");
  if (!subscribeNotification(NOT1_SVC_UUID, NOT1_CHR_UUID)) {
    return false;
  }
  m_Progress = 65;

  ESP_LOGI(LOG_TAG, "Subscribing to notification 2");
  if (!subscribeNotification(NOT1_SVC_UUID, NOT1_CHR_UUID)) {
    return false;
  }
  m_Progress = 70;

  ESP_LOGI(LOG_TAG, "Subscribing to notification 3");
  if (!subscribeNotification(NOT1_SVC_UUID, NOT1_CHR_UUID)) {
    return false;
  }
  m_Progress = 75;

  ESP_LOGI(LOG_TAG, "Subscribing to notification 4");
  if (!subscribeNotification(NOT1_SVC_UUID, NOT1_CHR_UUID)) {
    return false;
  }

  ESP_LOGI(LOG_TAG, "Subscribing to notification 5");
  if (!subscribeNotification(NOT1_SVC_UUID, NOT1_CHR_UUID)) {
    return false;
  }
  m_Progress = 80;

  ESP_LOGI(LOG_TAG, "Subscribing to notification 6");
  if (!subscribeNotification(NOT1_SVC_UUID, NOT1_CHR_UUID)) {
    return false;
  }
  m_Progress = 85;

  ESP_LOGI(LOG_TAG, "Getting shutter service");
  auto *pSvc = m_Client->getService(SHUTTER_SVC_UUID);
  if (pSvc == nullptr) {
    ESP_LOGI(LOG_TAG, "Failed to get shutter service");
    return false;
  }
  m_Progress = 90;

  ESP_LOGI(LOG_TAG, "Getting shutter characteristic");
  m_Shutter = pSvc->getCharacteristic(CHR_SHUTTER_UUID);
  if (m_Shutter == nullptr) {
    ESP_LOGI(LOG_TAG, "Failed to get shutter characteristic");
    return false;
  }
  m_Progress = 100;

  return true;
}

size_t FujifilmSecure::getSerialisedBytes(void) const {
  return sizeof(nvs_t);
}

bool FujifilmSecure::serialise(void *buffer, size_t bytes) const {
  if (bytes != sizeof(nvs_t)) {
    return false;
  }
  nvs_t *x = static_cast<nvs_t *>(buffer);
  strncpy(x->name, m_Name.c_str(), MAX_NAME);
  x->address = (uint64_t)m_Address;
  x->type = m_Address.getType();

  return true;
}

}  // namespace Furble
