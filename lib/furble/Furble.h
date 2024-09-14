#ifndef FURBLE_H
#define FURBLE_H

#include <vector>

#include "CameraList.h"
#include "FurbleTypes.h"
#include "HIDServer.h"

#ifndef FURBLE_VERSION
#define FURBLE_VERSION "unknown"
#endif

typedef void(scanResultCallback(void *context));

namespace Furble {
/**
 * BLE advertisement scanning class.
 *
 * Works in conjunction with Furble::Device class.
 */
class Scan {
 public:
  /**
   * Initialise the BLE scanner.
   */
  static void init(esp_power_level_t power);

  /**
   * Start the scan for BLE advertisements with a callback function when a matching reseult is
   * encountered.
   */
  static void start(scanResultCallback scanCallBack, void *scanResultPrivateData);

  /**
   * Stop the scan.
   */
  static void stop(void);

  /**
   * Clear the scan list.
   */
  static void clear(void);

  Scan();
  ~Scan();

 private:
  class ScanCallback;
  class HIDServerCallback;

  static NimBLEScan *m_Scan;
  static scanResultCallback *m_ScanResultCallback;
  static void *m_ScanResultPrivateData;
  static HIDServer *m_HIDServer;
};

}  // namespace Furble

#endif
