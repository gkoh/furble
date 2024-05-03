#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>

#include "Device.h"
#include "Furble.h"

namespace Furble {

NimBLEScan *Scan::m_Scan;
scanResultCallback *Scan::m_ScanResultCallback = nullptr;
void *Scan::m_ScanResultPrivateData = nullptr;

void scanEndCB(NimBLEScanResults results) {
  Serial.println("Scan ended");
}

/**
 * BLE Advertisement callback.
 */
class Scan::AdvertisedCallback: public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *pDevice) {
    if (CameraList::match(pDevice)) {
      if (m_ScanResultCallback != nullptr) {
        (m_ScanResultCallback)(m_ScanResultPrivateData);
      }
    }
  }
};

void Scan::init(esp_power_level_t power) {
  NimBLEDevice::init(Device::getStringID());
  NimBLEDevice::setPower(power);
  NimBLEDevice::setSecurityAuth(true, true, true);
  Scan::m_Scan = NimBLEDevice::getScan();
  m_Scan->setAdvertisedDeviceCallbacks(new AdvertisedCallback());
  m_Scan->setActiveScan(true);
  m_Scan->setInterval(6553);
  m_Scan->setWindow(6553);
}

void Scan::start(const uint32_t scanDuration,
                 scanResultCallback scanCallback,
                 void *scanPrivateData) {
  m_ScanResultCallback = scanCallback;
  m_ScanResultPrivateData = scanPrivateData;
  m_Scan->start(scanDuration, scanEndCB, false);
}

void Scan::stop(void) {
  m_Scan->stop();
  m_ScanResultPrivateData = nullptr;
  m_ScanResultCallback = nullptr;
}

void Scan::clear(void) {
  m_Scan->clearResults();
}
}  // namespace Furble
