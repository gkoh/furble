#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>

#include "Device.h"
#include "Furble.h"

// log tag
const char *LOG_TAG = FURBLE_STR;

namespace Furble {

NimBLEScan *Scan::m_Scan = nullptr;
scanResultCallback *Scan::m_ScanResultCallback = nullptr;
void *Scan::m_ScanResultPrivateData = nullptr;
HIDServer *Scan::m_HIDServer = nullptr;

/**
 * BLE Advertisement callback.
 */
class Scan::ScanCallback: public NimBLEScanCallbacks {
  void onResult(NimBLEAdvertisedDevice *pDevice) {
    if (CameraList::match(pDevice)) {
      if (m_ScanResultCallback != nullptr) {
        (m_ScanResultCallback)(m_ScanResultPrivateData);
      }
    }
  }
};

/**
 * HID server callback.
 */
class Scan::HIDServerCallback: public HIDServerCallbacks {
  void onComplete(const NimBLEAddress &address, const std::string &name) {
    CameraList::add(address, name);
    if (m_ScanResultCallback != nullptr) {
      (m_ScanResultCallback)(m_ScanResultPrivateData);
    }
  }
};

void Scan::init(esp_power_level_t power) {
  NimBLEDevice::init(Device::getStringID());
  NimBLEDevice::setPower(power);
  NimBLEDevice::setSecurityAuth(true, true, true);

  // NimBLE requires configuring server before scan
  m_HIDServer = HIDServer::getInstance();

  Scan::m_Scan = NimBLEDevice::getScan();
  m_Scan->setScanCallbacks(new ScanCallback());
  m_Scan->setActiveScan(true);
  m_Scan->setInterval(6553);
  m_Scan->setWindow(6553);
}

void Scan::start(const uint32_t scanDuration,
                 scanResultCallback scanCallback,
                 void *scanPrivateData) {
  m_HIDServer->start(scanDuration, new HIDServerCallback());

  m_ScanResultCallback = scanCallback;
  m_ScanResultPrivateData = scanPrivateData;
  m_Scan->start(scanDuration, false);
}

void Scan::stop(void) {
  m_HIDServer->stop();

  m_Scan->stop();
  m_ScanResultPrivateData = nullptr;
  m_ScanResultCallback = nullptr;
}

void Scan::clear(void) {
  m_Scan->clearResults();
}
}  // namespace Furble
