#ifndef FURBLE_CONTROL_H
#define FURBLE_CONTROL_H

#include <Camera.h>

#define CONTROL_CMD_QUEUE_LEN (32)
#define TARGET_CMD_QUEUE_LEN (8)

typedef enum {
  CONTROL_CMD_SHUTTER_PRESS,
  CONTROL_CMD_SHUTTER_RELEASE,
  CONTROL_CMD_FOCUS_PRESS,
  CONTROL_CMD_FOCUS_RELEASE,
  CONTROL_CMD_GPS_UPDATE,
  CONTROL_CMD_DISCONNECT,
  CONTROL_CMD_ERROR
} control_cmd_t;

namespace Furble {

class Control {
 public:
  class Target {
   public:
    Target(Camera *camera);
    ~Target();

    Camera *getCamera(void);
    control_cmd_t getCommand(void);
    void sendCommand(control_cmd_t cmd);
    const Camera::gps_t &getGPS(void);
    const Camera::timesync_t &getTimesync(void);

    void updateGPS(Camera::gps_t &gps, Camera::timesync_t &timesync);

   private:
    QueueHandle_t m_Queue = NULL;
    Furble::Camera *m_Camera = NULL;
    Camera::gps_t m_GPS;
    Camera::timesync_t m_Timesync;
  };

  Control(void);
  ~Control();

  /**
   * FreeRTOS control task function.
   */
  void task(void);

  /**
   * Send control command to active connections.
   */
  BaseType_t sendCommand(control_cmd_t cmd);

  /**
   * Update GPS and timesync values.
   */
  BaseType_t updateGPS(Camera::gps_t &gps, Camera::timesync_t &timesync);

  /**
   * Are all active cameras still connected?
   */
  bool isConnected(void);

  /**
   * Get list of connected targets.
   */
  const std::vector<std::unique_ptr<Control::Target>> &getTargets(void);

  /**
   * Disconnect all connected cameras.
   */
  void disconnect(void);

  /**
   * Add specified camera to active target list.
   */
  void addActive(Camera *camera);

 private:
  QueueHandle_t m_Queue = NULL;
  std::vector<std::unique_ptr<Control::Target>> m_Targets;
};

};  // namespace Furble

extern "C" {
void control_task(void *param);
}

#endif