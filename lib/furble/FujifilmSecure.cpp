#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>

#include "Device.h"
#include "FujifilmSecure.h"
#include "Scan.h"

namespace Furble {

const NimBLEUUID FujifilmSecure::SERVICE_UUID {0xa9d2b304, 0xe8d6, 0x4902, 0x8336352b772d7597};
const NimBLEUUID FujifilmSecure::PRI_SVC_UUID {0x731893f9, 0x744e, 0x4899, 0xb7e3174106ff2b82};

/**
 * Determine if the advertised BLE device is a Fujifilm secure camera.
 */
bool FujifilmSecure::matches(const NimBLEAdvertisedDevice *pDevice) {
  return (Fujifilm::matches(pDevice)
          && (pDevice->getManufacturerData().length() == sizeof(adv_secure_t))
          && pDevice->isAdvertisingService(SERVICE_UUID));
}

FujifilmSecure::FujifilmSecure(const void *data, size_t len)
    : Fujifilm(Type::FUJIFILM_SECURE, data, len) {
  if (len != sizeof(nvs_t))
    abort();

  const nvs_t *fujifilm = static_cast<const nvs_t *>(data);
  m_Name = std::string(fujifilm->name);
  m_Address = NimBLEAddress(fujifilm->address, fujifilm->type);
  m_Serial = fujifilm->serial;
  m_Queue = xQueueCreate(3, sizeof(bool));
}

FujifilmSecure::FujifilmSecure(const NimBLEAdvertisedDevice *pDevice)
    : Fujifilm(Type::FUJIFILM_SECURE, pDevice) {
  const adv_secure_t secure = pDevice->getManufacturerData<adv_secure_t>();
  m_Name = pDevice->getName();
  m_Address = pDevice->getAddress();
  m_Serial = secure.serial;
  m_Queue = xQueueCreate(3, sizeof(bool));

  ESP_LOGI(LOG_TAG, "Name = %s", m_Name.c_str());
  ESP_LOGI(LOG_TAG, "Address = %s", m_Address.toString().c_str());
  ESP_LOGI(LOG_TAG, "Serial = %s",
           NimBLEUtils::dataToHexString(m_Serial.data, sizeof(m_Serial)).c_str());
}

FujifilmSecure::~FujifilmSecure(void) {
  vQueueDelete(m_Queue);
}

bool FujifilmSecure::subscribe(const NimBLEUUID &svc, const NimBLEUUID &chr, bool notification) {
  auto pSvc = m_Client->getService(svc);
  if (pSvc == nullptr) {
    return false;
  }

  auto pChr = pSvc->getCharacteristic(chr);
  if (pChr == nullptr) {
    return false;
  }

  return pChr->subscribe(
      notification,
      [this](BLERemoteCharacteristic *pChr, uint8_t *pData, size_t length, bool isNotify) {
        ESP_LOGI(LOG_TAG, "Notification received on %s", pChr->getUUID().toString().c_str());
        if (length > 0) {
          ESP_LOGI(LOG_TAG, " %s", NimBLEUtils::dataToHexString(pData, length).c_str());
        }
      },
      true);
}

void FujifilmSecure::onResult(const NimBLEAdvertisedDevice *pDevice) {
  if (pDevice->haveManufacturerData()) {
    ESP_LOGI(LOG_TAG, "onResult(), len(md) = %u, sizeof(adv_secure_t) = %u",
             pDevice->getManufacturerData().length(), sizeof(adv_secure_t));
  }
  if ((Fujifilm::matches(pDevice)
       && (pDevice->getManufacturerData().length() == sizeof(adv_secure_t))
       && pDevice->isAdvertisingService(PAIR_SVC_UUID))) {
    const adv_secure_t scan = pDevice->getManufacturerData<adv_secure_t>();
    ESP_LOGI(LOG_TAG, "got %s, want %s",
             NimBLEUtils::dataToHexString(scan.serial.data, sizeof(scan.serial.data)).c_str(),
             NimBLEUtils::dataToHexString(m_Serial.data, sizeof(m_Serial.data)).c_str());
    if (memcmp(&scan.serial, &m_Serial, sizeof(m_Serial)) == 0) {
      m_Address = pDevice->getAddress();
      bool success = true;
      xQueueSend(m_Queue, &success, 0);
    }
  }
}

/**
 * Connect to a Fujifilm secure.
 */
bool FujifilmSecure::_connect(void) {
  bool success = false;
  m_Progress = 0;

  if (m_PairType == PairType::SAVED) {
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

    if (!success) {
      ESP_LOGI(LOG_TAG, "Failed to scan paired camera");
      return false;
    }
  }

  ESP_LOGI(LOG_TAG, "Connecting to %s", m_Address.toString().c_str());
  if (!m_Client->connect(m_Address))
    return false;

  ESP_LOGI(LOG_TAG, "Connected");
  m_Progress += 5;

  ESP_LOGI(LOG_TAG, "Requesting status");
  auto status = m_Client->getValue(PAIR_SVC_UUID, STATUS_CHR_UUID);
  if (status.size() == 4) {
    ESP_LOGI(LOG_TAG, "Status: %s",
             NimBLEUtils::dataToHexString(status.data(), status.size()).c_str());
    const auto ack = NimBLEAttValue({status[0], status[1], status[2], 0x20});
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
  m_Progress += 5;

  auto name = NimBLEAttValue(Device::getStringID());
  ESP_LOGI(LOG_TAG, "Identifying as %s", name.c_str());
  if (!m_Client->setValue(PAIR_SVC_UUID, IDENT_CHR_UUID, name, true)) {
    ESP_LOGI(LOG_TAG, "Failed to send identifier");
    return false;
  }
  ESP_LOGI(LOG_TAG, "Identified!");
  m_Progress += 5;

  const std::array<sub_t, 7> subscription0 = {
      {
       {"indication 1", SVC_CONF_UUID, CHR_IND1_UUID, false},
       {"indication 2", SVC_CONF_UUID, CHR_IND2_UUID, false},
       {"notification 1", SVC_CONF_UUID, CHR_NOT1_UUID, true},
       {"notification 2", SVC_CONF_UUID, GEOTAG_UPDATE, true},
       {"notification 3", NOT3_SVC_UUID, NOT3_CHR_UUID, true},
       {"notification 4", NOTX_SVC_UUID, NOT4_CHR_UUID, true},
       {"notification 5", NOTX_SVC_UUID, NOT5_CHR_UUID, true},
       }
  };

  for (const auto &sub : subscription0) {
    ESP_LOGI(LOG_TAG, "Subscribing to %s", sub.name.c_str());
    if (!subscribe(sub.service, sub.uuid, sub.notification)) {
      return false;
    }
    m_Progress += 5;
  }

  ESP_LOGI(LOG_TAG, "Writing 0x01");
  if (!m_Client->setValue(NOTX_SVC_UUID, UNK0_CHR_UUID, {0x01}, true)) {
    ESP_LOGI(LOG_TAG, "Failed to write 0x01");
    return false;
  }

  // may need to send time sync message?
  // SVC: e872b11fd5264ae19bb489a99d48fa59
  // CHR: c52edbce1fe24ecc9483907e6592be9e}

  const std::array<sub_t, 6> subscription1 = {
      {
       {"notification 6", SVC_CONF_UUID, NOT6_CHR_UUID, true},
       {"notification 7", NOTX_SVC_UUID, NOT7_CHR_UUID, true},
       {"notification 8", NOTX_SVC_UUID, NOT8_CHR_UUID, true},
       {"notification 9", NOTX_SVC_UUID, NOT9_CHR_UUID, true},
       {"notification 10", NOTX_SVC_UUID, NOT10_CHR_UUID, true},
       {"notification 11", NOTX_SVC_UUID, GEOTAG_SYNC_INTERVAL_UUID, true},
       }
  };

  for (const auto &sub : subscription1) {
    ESP_LOGI(LOG_TAG, "Subscribing to %s", sub.name.c_str());
    if (!subscribe(sub.service, sub.uuid, sub.notification)) {
      ESP_LOGI(LOG_TAG, "Failed to subscribe to %s", sub.name.c_str());
      if (sub.uuid == GEOTAG_SYNC_INTERVAL_UUID) {
        // Geotag subscription must succeed
        return false;
      }
    }
    m_Progress += 5;
  }

  auto sync_interval = NimBLEAttValue(reinterpret_cast<const uint8_t *>(&GEOTAG_SYNC_INTERVAL),
                                      sizeof(GEOTAG_SYNC_INTERVAL));
  ESP_LOGI(LOG_TAG, "Configuring %hus geotag sync interval", GEOTAG_SYNC_INTERVAL);
  if (!m_Client->setValue(NOTX_SVC_UUID, GEOTAG_SYNC_INTERVAL_UUID, sync_interval, true)) {
    ESP_LOGI(LOG_TAG, "Failed to configure geotag sync interval");
    return false;
  }
  m_Progress += 5;

  ESP_LOGI(LOG_TAG, "Getting shutter service");
  auto *pSvc = m_Client->getService(SHUTTER_SVC_UUID);
  if (pSvc == nullptr) {
    ESP_LOGI(LOG_TAG, "Failed to get shutter service");
    return false;
  }
  m_Progress += 5;

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
  x->serial = m_Serial;

  return true;
}

}  // namespace Furble
