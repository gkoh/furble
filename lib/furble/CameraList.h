#ifndef CAMERALIST_H
#define CAMERALIST_H

#include "Camera.h"
#include "MobileDevice.h"

namespace Furble {

class CameraList {
 public:
  CameraList();
  ~CameraList();
  /**
   * Save camera to connection list.
   */
  static void save(Camera *pCamera);

  /**
   * Remove camera from connection list.
   */
  static void remove(Camera *pCamera);

  /**
   * Load previously connected devices.
   */
  static void load(void);

  /**
   * Get number of saved connections.
   */
  static size_t getSaveCount(void);

  /**
   * Add matching devices to the list.
   *
   * @return true if device matches
   */
  static bool match(NimBLEAdvertisedDevice *pDevice);

  /**
   * Add mobile device to the list.
   */
  static void add(NimBLEAddress address);

  /**
   * Number of connectable devices.
   */
  static size_t size(void);

  /**
   * Clear connectable devices.
   */
  static void clear(void);

  /**
   * Retrieve device by index.
   */
  static Furble::Camera *get(size_t n);

  /**
   * Retrieve last entry.
   */
  static Camera *back(void);

 private:
  /**
   * List of connectable devices.
   */
  static std::vector<Furble::Camera *> m_ConnectList;
};
}  // namespace Furble

#endif
