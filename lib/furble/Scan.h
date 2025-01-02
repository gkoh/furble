#ifndef SCAN_H
#define SCAN_H

#include <vector>

#include <NimBLEScan.h>

#include "CameraList.h"
#include "FurbleTypes.h"
#include "HIDServer.h"

#ifndef FURBLE_VERSION
#define FURBLE_VERSION "unknown"
#endif

namespace Furble {
/**
 * BLE advertisement scanning class.
 *
 * Works in conjunction with Furble::Device class.
 */
class Scan: public HIDServerCallbacks, public NimBLEScanCallbacks {
 public:
  static Scan &getInstance(void);

  Scan(Scan const &) = delete;
  Scan(Scan &&) = delete;
  Scan &operator=(Scan const &) = delete;
  Scan &operator=(Scan &&) = delete;

  /**
   * Start the scan for BLE advertisements with a callback function when a
   * matching reseult is encountered.
   */
  void start(std::function<void(void *)> scanCallback, void *scanResultPrivateData);

  /**
   * Stop the scan.
   */
  void stop(void);

  /**
   * Clear the scan list.
   */
  void clear(void);

  void onResult(const NimBLEAdvertisedDevice *pDevice) override;

  void onComplete(const NimBLEAddress &address, const std::string &name) override;

 private:
  Scan() {};

  NimBLEScan *m_Scan = nullptr;
  std::function<void(void *)> m_ScanResultCallback;
  void *m_ScanResultPrivateData = nullptr;
  HIDServer *m_HIDServer = nullptr;
};

}  // namespace Furble

#endif
