#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>

#include <NimBLEAddress.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEConnInfo.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>
#include <NimBLEUtils.h>

#include "Ricoh.h"

namespace Furble {

const NimBLEUUID Ricoh::INFO_SVC_UUID {0x9A5ED1C5, 0x74CC, 0x4C50, 0xB5B666A48E7CCFF1};
const NimBLEUUID Ricoh::MODEL_CHR_UUID {0x35FE6272, 0x6AA5, 0x44D9, 0x88E1F09427F51A71};

const NimBLEUUID Ricoh::CAMERA_SVC_UUID {0x4B445988, 0xCAA0, 0x4DD3, 0x941D37B4F52ACA86};
const NimBLEUUID Ricoh::POWER_CHR_UUID {0xB58CE84C, 0x0666, 0x4DE9, 0xBEC82D27B27B3211};
const NimBLEUUID Ricoh::OPERATION_MODE_CHR_UUID {0x1452335A, 0xEC7F, 0x4877, 0xB8AB0F72E18BB295};

const NimBLEUUID Ricoh::SHOOTING_SVC_UUID {0x9F00F387, 0x8345, 0x4BBC, 0x8B92B87B52E3091A};
const NimBLEUUID Ricoh::SHOOTING_FLAVOR_CHR_UUID {0xB29E6DE3, 0x1AEC, 0x48C1, 0x9D0502CEA57CE664};
const NimBLEUUID Ricoh::OPERATION_REQUEST_CHR_UUID {0x559644B8, 0xE0BC, 0x4011, 0x929B5CF9199851E7};
const NimBLEUUID Ricoh::CAPTURE_STATUS_CHR_UUID {0xB5589C08, 0xB5FD, 0x46F5, 0xBE7DAB1B8C074CAA};
const NimBLEUUID Ricoh::SELF_TIMER_CHR_UUID {0x009A8E70, 0xB306, 0x4451, 0xB9437F54392EB971};

const NimBLEUUID Ricoh::BT_CONTROL_SVC_UUID {0x0F291746, 0x0C80, 0x4726, 0x87A73C501FD3B4B6};
const NimBLEUUID Ricoh::PAIRED_DEVICE_NAME_CHR_UUID {0xFE3A32F8, 0xA189, 0x42DE,
                                                     0xA391BC81AE4DAA76};

const NimBLEUUID Ricoh::GPS_SVC_UUID {0x84A0DD62, 0xE8AA, 0x4D0F, 0x91DB819B6724C69E};
const NimBLEUUID Ricoh::GPS_INFO_CHR_UUID {0x28F59D60, 0x8B8E, 0x4FCD, 0xA81F61BDB46595A9};

const NimBLEUUID Ricoh::LOCATION_CONTROL_SVC_UUID {0xF37F568F, 0x9071, 0x445D, 0xA9385441F2E82399};
const NimBLEUUID Ricoh::LOCATION_CONTROL_CHR_UUID {0x9111CDD0, 0x9F01, 0x45C4, 0xA2D4E09E8FB0424D};

namespace {

bool validTimesync(const Camera::timesync_t &timesync) {
  return timesync.year >= 2000 && timesync.year <= 2099 && timesync.month >= 1
         && timesync.month <= 12 && timesync.day >= 1 && timesync.day <= 31 && timesync.hour <= 23
         && timesync.minute <= 59 && timesync.second <= 60 && timesync.centisecond <= 99;
}

const char *powerName(uint8_t value) {
  switch (value) {
    case 0:
      return "OFF";
    case 1:
      return "ON";
    case 2:
      return "SLEEP";
    default:
      return "?";
  }
}

const char *opModeName(uint8_t value) {
  switch (value) {
    case 0:
      return "CAPTURE";
    case 1:
      return "PLAYBACK";
    case 2:
      return "BLE_STARTUP";
    case 3:
      return "OTHER";
    case 4:
      return "POWER_OFF_TRANSFER";
    default:
      return "?";
  }
}

void logChr(NimBLERemoteCharacteristic *pChr, const char *label,
            const char *(*decode)(uint8_t) = nullptr) {
  if (pChr == nullptr) {
    ESP_LOGI(LOG_TAG, "Ricoh %s: missing", label);
    return;
  }
  ESP_LOGI(LOG_TAG, "Ricoh %s uuid=%s read=%d write=%d writeNoRsp=%d notify=%d indicate=%d", label,
           pChr->getUUID().toString().c_str(), pChr->canRead(), pChr->canWrite(),
           pChr->canWriteNoResponse(), pChr->canNotify(), pChr->canIndicate());
  if (!pChr->canRead())
    return;
  NimBLEAttValue value = pChr->readValue();
  if (value.length() == 0) {
    ESP_LOGI(LOG_TAG, "Ricoh %s value=<empty>", label);
    return;
  }
  if (decode != nullptr && value.length() == 1)
    ESP_LOGI(LOG_TAG, "Ricoh %s value=0x%02X decoded=%s", label, value.data()[0],
             decode(value.data()[0]));
  else
    ESP_LOGI(LOG_TAG, "Ricoh %s value=%s", label,
             NimBLEUtils::dataToHexString(value.data(), static_cast<uint8_t>(value.length()))
                 .c_str());
}

}  // namespace

Ricoh::Ricoh(const void *data, size_t len) : Camera(Type::RICOH, PairType::SAVED) {
  if (len != sizeof(ricoh_t))
    abort();

  const ricoh_t *ricoh = static_cast<const ricoh_t *>(data);
  m_Name = std::string(ricoh->name);
  m_Address = NimBLEAddress(ricoh->address, ricoh->type);
}

Ricoh::Ricoh(const NimBLEAdvertisedDevice *pDevice) : Camera(Type::RICOH, PairType::NEW) {
  m_Name = pDevice->getName();
  if (m_Name.empty())
    m_Name = "RICOH";
  m_Address = pDevice->getAddress();
  ESP_LOGI(LOG_TAG, "Ricoh Name = %s", m_Name.c_str());
  ESP_LOGI(LOG_TAG, "Ricoh Address = %s", m_Address.toString().c_str());
}

bool Ricoh::nameMatches(const std::string &name) {
  if (name.empty())
    return false;

  std::string upper = name;
  std::transform(upper.begin(), upper.end(), upper.begin(),
                 [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

  const std::array<const char *, 5> matches = {"RICOH", "PENTAX", "GR ", "GRIII", "GR III"};
  return upper == "GR" || std::any_of(matches.begin(), matches.end(), [&upper](const char *match) {
           return upper.find(match) != std::string::npos;
         });
}

bool Ricoh::matches(const NimBLEAdvertisedDevice *pDevice) {
  return pDevice->isAdvertisingService(INFO_SVC_UUID)
         || pDevice->isAdvertisingService(CAMERA_SVC_UUID)
         || pDevice->isAdvertisingService(SHOOTING_SVC_UUID)
         || pDevice->isAdvertisingService(BT_CONTROL_SVC_UUID) || nameMatches(pDevice->getName());
}

bool Ricoh::_connect(void) {
  m_Progress = 0;

  constexpr uint8_t kDefaultIOCap = BLE_HS_IO_DISPLAY_YESNO;
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_DISPLAY);
  NimBLEDevice::setSecurityAuth(true, true, true);

  bool bondedBefore = NimBLEDevice::isBonded(m_Address);
  ESP_LOGI(LOG_TAG, "Ricoh bonded(before)=%s pairType=%s", bondedBefore ? "yes" : "no",
           m_PairType == PairType::NEW ? "new" : "saved");

  ESP_LOGI(LOG_TAG, "Ricoh connecting");
  if (!m_Client->connect(m_Address)) {
    ESP_LOGI(LOG_TAG, "Ricoh connection failed");
    NimBLEDevice::setSecurityIOCap(kDefaultIOCap);
    return false;
  }
  m_Progress = 15;

  ESP_LOGI(LOG_TAG, "Ricoh securing");
  if (!m_Client->secureConnection()) {
    ESP_LOGW(LOG_TAG, "Ricoh secure connection failed (bonded before=%s)",
             bondedBefore ? "yes" : "no");
    NimBLEDevice::setSecurityIOCap(kDefaultIOCap);
    return false;
  }
  {
    NimBLEConnInfo connInfo = m_Client->getConnInfo();
    ESP_LOGI(LOG_TAG, "Ricoh secure: encrypted=%d authenticated=%d bonded=%d keySize=%u",
             connInfo.isEncrypted(), connInfo.isAuthenticated(), connInfo.isBonded(),
             connInfo.getSecKeySize());
    if (!connInfo.isEncrypted() || !connInfo.isAuthenticated() || !connInfo.isBonded()) {
      ESP_LOGW(LOG_TAG, "Ricoh secure link incomplete");
      NimBLEDevice::setSecurityIOCap(kDefaultIOCap);
      return false;
    }
  }
  m_Progress = 30;

  NimBLERemoteService *pSvc = m_Client->getService(INFO_SVC_UUID);
  if (pSvc != nullptr) {
    NimBLERemoteCharacteristic *pModel = pSvc->getCharacteristic(MODEL_CHR_UUID);
    if (pModel != nullptr && pModel->canRead()) {
      std::string model = static_cast<std::string>(pModel->readValue());
      if (!model.empty()) {
        ESP_LOGI(LOG_TAG, "Ricoh Model = %s", model.c_str());
        m_Name = model;
      }
    }
  }
  m_Progress = 45;

  pSvc = m_Client->getService(CAMERA_SVC_UUID);
  if (pSvc != nullptr) {
    m_Power = pSvc->getCharacteristic(POWER_CHR_UUID);
    m_OperationMode = pSvc->getCharacteristic(OPERATION_MODE_CHR_UUID);
  } else {
    ESP_LOGW(LOG_TAG, "Ricoh Camera service unavailable");
  }
  m_Progress = 60;

  pSvc = m_Client->getService(SHOOTING_SVC_UUID);
  if (pSvc == nullptr) {
    ESP_LOGW(LOG_TAG, "Ricoh Shooting service unavailable");
    NimBLEDevice::setSecurityIOCap(kDefaultIOCap);
    return false;
  }

  m_OperationRequest = pSvc->getCharacteristic(OPERATION_REQUEST_CHR_UUID);
  m_ShootingFlavor = pSvc->getCharacteristic(SHOOTING_FLAVOR_CHR_UUID);
  m_CaptureStatus = pSvc->getCharacteristic(CAPTURE_STATUS_CHR_UUID);
  m_SelfTimer = pSvc->getCharacteristic(SELF_TIMER_CHR_UUID);
  if (m_OperationRequest == nullptr || !m_OperationRequest->canWrite()) {
    ESP_LOGW(LOG_TAG, "Ricoh OperationRequest unavailable");
    NimBLEDevice::setSecurityIOCap(kDefaultIOCap);
    return false;
  }
  if (m_ShootingFlavor == nullptr || !m_ShootingFlavor->canWrite()) {
    ESP_LOGW(LOG_TAG, "Ricoh ShootingFlavor unavailable");
    NimBLEDevice::setSecurityIOCap(kDefaultIOCap);
    return false;
  }
  m_Progress = 75;

  pSvc = m_Client->getService(GPS_SVC_UUID);
  if (pSvc != nullptr) {
    m_GpsInfo = pSvc->getCharacteristic(GPS_INFO_CHR_UUID);
  } else {
    ESP_LOGW(LOG_TAG, "Ricoh GPS service unavailable");
  }
  m_Progress = 82;

  pSvc = m_Client->getService(LOCATION_CONTROL_SVC_UUID);
  if (pSvc != nullptr) {
    m_LocationControl = pSvc->getCharacteristic(LOCATION_CONTROL_CHR_UUID);
  } else {
    ESP_LOGW(LOG_TAG, "Ricoh Location Control service unavailable");
  }

  pSvc = m_Client->getService(BT_CONTROL_SVC_UUID);
  if (pSvc != nullptr) {
    m_PairedDeviceName = pSvc->getCharacteristic(PAIRED_DEVICE_NAME_CHR_UUID);
  } else {
    ESP_LOGW(LOG_TAG, "Ricoh Bluetooth Control service unavailable");
  }
  m_Progress = 88;

  ESP_LOGI(LOG_TAG, "Ricoh state probe begin");
  logChr(m_Power, "CameraPower", powerName);
  logChr(m_OperationMode, "OperationMode", opModeName);
  logChr(m_ShootingFlavor, "ShootingFlavor");
  logChr(m_OperationRequest, "OperationRequest");
  logChr(m_CaptureStatus, "CaptureStatus");
  logChr(m_SelfTimer, "SelfTimer");
  logChr(m_GpsInfo, "GpsInfo");
  logChr(m_LocationControl, "LocationControl");
  logChr(m_PairedDeviceName, "PairedDeviceName");
  ESP_LOGI(LOG_TAG, "Ricoh state probe end");

  subscribeCharacteristic(m_CaptureStatus, "CaptureStatus");
  subscribeCharacteristic(m_SelfTimer, "SelfTimer");
  subscribeCharacteristic(m_Power, "CameraPower");
  subscribeCharacteristic(m_OperationMode, "OperationMode");

  NimBLEDevice::setSecurityIOCap(kDefaultIOCap);

  m_Progress = 100;
  ESP_LOGI(LOG_TAG, "Ricoh connected");
  return true;
}

void Ricoh::_disconnect(void) {
  clearRemoteState();
  if (m_Client != nullptr && m_Client->isConnected())
    m_Client->disconnect();
}

void Ricoh::clearRemoteState(void) {
  m_Power = nullptr;
  m_OperationMode = nullptr;
  m_ShootingFlavor = nullptr;
  m_OperationRequest = nullptr;
  m_CaptureStatus = nullptr;
  m_SelfTimer = nullptr;
  m_PairedDeviceName = nullptr;
  m_GpsInfo = nullptr;
  m_LocationControl = nullptr;
}

bool Ricoh::writeByte(NimBLERemoteCharacteristic *pChr, uint8_t value, const char *label) {
  if (!isConnected()) {
    ESP_LOGW(LOG_TAG, "Ricoh %s skipped: not connected", label);
    return false;
  }
  if (pChr == nullptr || !pChr->canWrite()) {
    ESP_LOGW(LOG_TAG, "Ricoh %s unavailable", label);
    return false;
  }
  bool rc = pChr->writeValue(&value, sizeof(value), true);
  ESP_LOGD(LOG_TAG, "Ricoh %s = %s", label, rc ? "ok" : "failed");
  return rc;
}

bool Ricoh::writeOperation(OperationCode code, OperationParameter parameter) {
  if (!isConnected()) {
    ESP_LOGW(LOG_TAG, "Ricoh OperationRequest skipped: not connected");
    return false;
  }
  if (m_OperationRequest == nullptr) {
    ESP_LOGW(LOG_TAG, "Ricoh OperationRequest unavailable");
    return false;
  }
  const std::array<uint8_t, 2> cmd = {static_cast<uint8_t>(code), static_cast<uint8_t>(parameter)};
  bool rc = m_OperationRequest->writeValue(cmd.data(), cmd.size(), true);
  ESP_LOGI(LOG_TAG, "Ricoh OperationRequest code=%u param=%u => %s", static_cast<unsigned>(code),
           static_cast<unsigned>(parameter), rc ? "ok" : "failed");
  return rc;
}

bool Ricoh::subscribeCharacteristic(NimBLERemoteCharacteristic *pChr, const char *label) {
  if (pChr == nullptr || !pChr->canNotify()) {
    ESP_LOGD(LOG_TAG, "Ricoh %s subscribe skipped", label);
    return false;
  }
  bool rc = pChr->subscribe(
      true,
      [](NimBLERemoteCharacteristic *chr, uint8_t *data, size_t len, bool isNotify) {
        ESP_LOGI(LOG_TAG, "Ricoh notify %s (%s): %s", chr->getUUID().toString().c_str(),
                 isNotify ? "notify" : "indicate",
                 NimBLEUtils::dataToHexString(data, static_cast<uint8_t>(len)).c_str());
      },
      true);
  ESP_LOGI(LOG_TAG, "Ricoh subscribe %s => %s", label, rc ? "ok" : "failed");
  return rc;
}

bool Ricoh::setShootingFlavor(ShootingFlavor flavor) {
  return writeByte(m_ShootingFlavor, static_cast<uint8_t>(flavor), "ShootingFlavor");
}

void Ricoh::shutterPress(void) {
  if (!setShootingFlavor(ShootingFlavor::IMMEDIATE))
    return;
  writeOperation(OperationCode::START, OperationParameter::AF);
}

void Ricoh::shutterRelease(void) {
  ESP_LOGI(LOG_TAG, "Ricoh shutterRelease ignored (single-write capture)");
}

void Ricoh::focusPress(void) {
  if (!setShootingFlavor(ShootingFlavor::TIMER_2S))
    return;
  writeOperation(OperationCode::START, OperationParameter::AF);
}

void Ricoh::focusRelease(void) {
  ESP_LOGI(LOG_TAG, "Ricoh focusRelease ignored (single-write capture)");
}

void Ricoh::updateGeoData(const gps_t &gps, const timesync_t &timesync) {
  if (!isConnected()) {
    ESP_LOGW(LOG_TAG, "Ricoh GPS skipped: not connected");
    return;
  }
  if (!std::isfinite(gps.latitude) || !std::isfinite(gps.longitude) || !std::isfinite(gps.altitude)
      || !validTimesync(timesync)) {
    ESP_LOGW(LOG_TAG,
             "Ricoh GPS invalid: lat=%.7f lon=%.7f alt=%.1f "
             "utc=%04u-%02u-%02u %02u:%02u:%02u.%02u",
             gps.latitude, gps.longitude, gps.altitude, timesync.year, timesync.month, timesync.day,
             timesync.hour, timesync.minute, timesync.second, timesync.centisecond);
    return;
  }
  ESP_LOGD(LOG_TAG,
           "Ricoh GPS valid (stub): lat=%.7f lon=%.7f alt=%.1f "
           "utc=%04u-%02u-%02u %02u:%02u:%02u.%02u",
           gps.latitude, gps.longitude, gps.altitude, timesync.year, timesync.month, timesync.day,
           timesync.hour, timesync.minute, timesync.second, timesync.centisecond);
}

size_t Ricoh::getSerialisedBytes(void) const {
  return sizeof(ricoh_t);
}

bool Ricoh::serialise(void *buffer, size_t bytes) const {
  if (bytes != sizeof(ricoh_t))
    return false;

  ricoh_t *x = static_cast<ricoh_t *>(buffer);
  strncpy(x->name, m_Name.c_str(), MAX_NAME);
  x->name[MAX_NAME - 1] = '\0';
  x->address = static_cast<uint64_t>(m_Address);
  x->type = m_Address.getType();

  return true;
}

void Ricoh::onPassKeyEntry(NimBLEConnInfo &connInfo) {
  ESP_LOGW(LOG_TAG, "Ricoh passkey entry for %s; injecting fallback 123456",
           connInfo.getAddress().toString().c_str());
  NimBLEDevice::injectPassKey(connInfo, 123456);
}

uint32_t Ricoh::onPassKeyDisplay(NimBLEConnInfo &connInfo) {
  ESP_LOGW(LOG_TAG, "Ricoh passkey display for %s; returning fallback 123456",
           connInfo.getAddress().toString().c_str());
  return 123456;
}

void Ricoh::onConfirmPasskey(NimBLEConnInfo &connInfo, uint32_t pin) {
  ESP_LOGI(LOG_TAG, "Ricoh confirm passkey %06lu for %s", pin,
           connInfo.getAddress().toString().c_str());
  NimBLEDevice::injectConfirmPasskey(connInfo, true);
}

void Ricoh::onAuthenticationComplete(NimBLEConnInfo &connInfo) {
  ESP_LOGI(LOG_TAG, "Ricoh auth complete: bonded=%d encrypted=%d authenticated=%d keySize=%u",
           connInfo.isBonded(), connInfo.isEncrypted(), connInfo.isAuthenticated(),
           connInfo.getSecKeySize());
}

}  // namespace Furble
