#include <NimBLEAdvertisedDevice.h>
#include <NimBLEDevice.h>

#include "Device.h"
#include "Furble.h"

namespace Furble {

NimBLEScan *Scan::m_Scan;
scanResultCallback *Scan::m_ScanResultCallback = nullptr;

/**
 * BLE Advertisement callback.
 */
class Scan::AdvertisedCallback: public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *pDevice) {
    CameraList::match(pDevice);
    if (m_ScanResultCallback != nullptr) {
      (m_ScanResultCallback)(CameraList::m_ConnectList);
    }
  }
};

void Scan::init(esp_power_level_t power, scanResultCallback scanCallback) {
  m_ScanResultCallback = scanCallback;
  NimBLEDevice::init(Device::getStringID());
  NimBLEDevice::setPower(power);
  NimBLEDevice::setSecurityAuth(true, true, true);
  Scan::m_Scan = NimBLEDevice::getScan();
  m_Scan->setAdvertisedDeviceCallbacks(new AdvertisedCallback());
  m_Scan->setActiveScan(true);
  m_Scan->setInterval(6553);
  m_Scan->setWindow(6553);
}

void Scan::start(const uint32_t scanDuration) {
  m_Scan->start(scanDuration, false);
}

void Scan::clear(void) {
  m_Scan->clearResults();
}
}  // namespace Furble
