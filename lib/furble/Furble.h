#ifndef FURBLE_H
#define FURBLE_H

#include <NimBLEAdvertisedDevice.h>
#include <NimBLEClient.h>
#include <Preferences.h>
#include <vector>

#include "CameraList.h"
#include "FurbleTypes.h"

#ifndef FURBLE_VERSION
#define FURBLE_VERSION "unknown"
#endif

typedef void(scanResultCallback(std::vector<Furble::Camera *> &list));

namespace Furble {
/**
 * BLE advertisement scanning class.
 *
 * Works in conjunction with Furble::Device class.
 */
class Scan {
 public:
  /**
   * Initialise the BLE scanner with a callback function when a matching result is encountered.
   */
  static void init(scanResultCallback scanCallBack);

  /**
   * Start the scan for BLE advertisements.
   */
  static void start(const uint32_t scanDuration);

  /**
   * Clear the scan list.
   */
  static void clear(void);

  Scan();
  ~Scan();

 private:
  class AdvertisedCallback;
  static NimBLEScan *m_Scan;
  static scanResultCallback *m_ScanResultCallback;
};

/**
 * Set the BLE transmit power.
 */
void setPower(esp_power_level_t power);
}  // namespace Furble

#endif
