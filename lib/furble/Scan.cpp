#include <NimBLEAdvertisedDevice.h>
#include <NimBLEScan.h>

#include "Device.h"
#include "Scan.h"

// log tag
const char *LOG_TAG = FURBLE_STR;

namespace Furble {

Scan &Scan::getInstance(void) {
  static Scan instance;

  if (instance.m_Scan == nullptr) {
    // NimBLE requires configuring server before scan
    instance.m_HIDServer = HIDServer::getInstance();

    instance.m_Scan = NimBLEDevice::getScan();
    instance.m_Scan->setActiveScan(true);
    instance.m_Scan->setInterval(6553);
    instance.m_Scan->setWindow(6553);
  }

  return instance;
}

/**
 * BLE Advertisement callback.
 */
void Scan::onResult(const NimBLEAdvertisedDevice *pDevice) {
  if (CameraList::match(pDevice)) {
    ESP_LOGI(LOG_TAG, "RSSI(%s) = %d", pDevice->getName().c_str(), pDevice->getRSSI());
    if (m_ScanResultCallback != nullptr) {
      (m_ScanResultCallback)(m_ScanResultPrivateData);
    }
  }
};

/**
 * HID server callback.
 */
void Scan::onComplete(const NimBLEAddress &address, const std::string &name) {
  CameraList::add(address, name);
  if (m_ScanResultCallback != nullptr) {
    (m_ScanResultCallback)(m_ScanResultPrivateData);
  }
};

void Scan::start(std::function<void(void *)> scanCallback, void *scanPrivateData) {
  m_HIDServer->start(nullptr, this);

  m_Scan->setScanCallbacks(this);

  m_ScanResultCallback = scanCallback;
  m_ScanResultPrivateData = scanPrivateData;
  m_Scan->start(0, false);
}

void Scan::start(NimBLEScanCallbacks *pScanCallbacks, uint32_t duration) {
  m_Scan->setScanCallbacks(pScanCallbacks);
  m_Scan->start(duration, false);
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
