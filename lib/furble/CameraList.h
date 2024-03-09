#ifndef CAMERALIST_H
#define CAMERALIST_H

#include "Camera.h"

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
   * Add matching devices to the list.
   */
  static void match(NimBLEAdvertisedDevice *pDevice);

  /**
   * List of connectable devices.
   */
  static std::vector<Furble::Camera *> m_ConnectList;
};
}  // namespace Furble

#endif
