#include <NimBLEScan.h>

#include "Device.h"
#include "Scan.h"

// log tag
const char *LOG_TAG = FURBLE_STR;

namespace Furble {

Scan &Scan::getInstance(void) {
  static Scan instance;

  if (instance.m_Scan == nullptr) {
    instance.m_Server = NimBLEDevice::createServer();

    instance.m_Scan = NimBLEDevice::getScan();
    instance.m_Scan->setActiveScan(true);
    instance.m_Scan->setInterval(6553);
    instance.m_Scan->setWindow(6553);
#ifdef FURBLE_XTEINK_X3
    // Callbacks copy the matching camera data; retain no advertiser history.
    instance.m_Scan->setMaxResults(0);
#endif
  }

  return instance;
}

/**
 * BLE Advertisement callback.
 */
void Scan::onResult(const NimBLEAdvertisedDevice *pDevice) {
  std::function<void(void *)> callback;
  void *privateData = nullptr;
  {
    const std::lock_guard<std::mutex> lock(m_ResultMutex);
    if (!m_AcceptingResults || !CameraList::match(pDevice))
      return;
    ESP_LOGI(LOG_TAG, "RSSI(%s) = %d", pDevice->getName().c_str(), pDevice->getRSSI());
    callback = m_ScanResultCallback;
    privateData = m_ScanResultPrivateData;
  }
  // Legacy UI callbacks acquire their own display mutex. Invoking here while
  // holding m_ResultMutex would deadlock a UI thread calling stop().
  if (callback)
    callback(privateData);
};

void Scan::start(std::function<void(void *)> scanCallback, void *scanPrivateData) {
  m_Server->start();
  m_Scan->setScanCallbacks(this);

  {
    const std::lock_guard<std::mutex> lock(m_ResultMutex);
    m_ScanResultCallback = scanCallback;
    m_ScanResultPrivateData = scanPrivateData;
    m_AcceptingResults = true;
  }
  m_Scan->start(0, false);
}

void Scan::start(NimBLEScanCallbacks *pScanCallbacks, uint32_t duration) {
  m_Scan->setScanCallbacks(pScanCallbacks);
  m_Scan->start(duration, false);
}

void Scan::stop(void) {
  {
    // Drain list mutation before cancellation. A copied notification callback
    // may still finish after stop; it must not own transient UI storage.
    const std::lock_guard<std::mutex> lock(m_ResultMutex);
    m_AcceptingResults = false;
    m_ScanResultPrivateData = nullptr;
    m_ScanResultCallback = nullptr;
  }
  m_Scan->stop();
}

bool Scan::isActive(void) const {
  return m_Scan->isScanning();
}

void Scan::clear(void) {
  m_Scan->clearResults();
}
}  // namespace Furble
