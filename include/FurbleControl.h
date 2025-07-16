#ifndef FURBLE_CONTROL_H
#define FURBLE_CONTROL_H

#include <memory>
#include <mutex>

#include <Camera.h>

namespace Furble {

class Control {
 public:
  typedef enum {
    CMD_SHUTTER_PRESS,
    CMD_SHUTTER_RELEASE,
    CMD_FOCUS_PRESS,
    CMD_FOCUS_RELEASE,
    CMD_GPS_UPDATE,
    CMD_CONNECT,
    CMD_DISCONNECT,
    CMD_ERROR
  } cmd_t;

  typedef enum {
    /** No connections, waiting. */
    STATE_IDLE,
    /** Initiate connections. */
    STATE_CONNECT,
    /** Connections in progress. */
    STATE_CONNECTING,
    /** Initial connection attempt failed. */
    STATE_CONNECT_FAILED,
    /** All connections active. */
    STATE_ACTIVE
  } state_t;

  class Target {
    friend class Control;

   public:
    ~Target();

    Camera *getCamera(void) const;
    cmd_t getCommand(void);
    void sendCommand(cmd_t cmd);
    void updateGPS(Camera::gps_t &gps, Camera::timesync_t &timesync);

    void task(void);

   protected:
    Target(Camera *camera);

    volatile bool m_Stopped = false;

   private:
    static constexpr UBaseType_t m_QueueLength = 8;

    QueueHandle_t m_Queue = NULL;
    Furble::Camera *m_Camera = NULL;
    Camera::gps_t m_GPS;
    Camera::timesync_t m_Timesync;
  };

  static Control &getInstance();

  Control(Control const &) = delete;
  Control(Control &&) = delete;
  Control &operator=(Control const &) = delete;
  Control &operator=(Control &&) = delete;

  const uint32_t TIMEOUTMS_DEFAULT = (30 * 1000);
  const uint32_t TIMEOUTMS_INFINITE = (10 * 1000);

  /**
   * FreeRTOS control task function.
   */
  void task(void);

  /**
   * Send control command to active connections.
   */
  BaseType_t sendCommand(cmd_t cmd);

  /**
   * Update GPS and timesync values.
   */
  BaseType_t updateGPS(Camera::gps_t &gps, Camera::timesync_t &timesync);

  /**
   * Are all active cameras still connected?
   */
  bool allConnected(void);

  /**
   * Get list of connected targets.
   */
  const std::vector<std::unique_ptr<Control::Target>> &getTargets(void);

  /**
   * Connect to all active cameras.
   */
  void connectAll(bool infiniteReconnect);

  /**
   * Disconnect all connected cameras.
   */
  void disconnect(void);

  /**
   * Add specified camera to active target list.
   */
  void addActive(Camera *camera);

  /**
   * Get current camera connection attempt.
   *
   * @return Camera being connected otherwise nullptr.
   */
  Camera *getConnectingCamera(void);

  /** Retrieve current control state. */
  state_t getState(void);

  /** Set transmit power. */
  void setPower(esp_power_level_t power);

 private:
  Control() {};

  /** Iterate over cameras and attempt connection. */
  state_t connectAll(void);

  static constexpr UBaseType_t m_QueueLength = 32;

  QueueHandle_t m_Queue = NULL;
  std::mutex m_Mutex;
  std::vector<std::unique_ptr<Control::Target>> m_Targets;

  bool m_InfiniteReconnect = false;
  state_t m_State = STATE_IDLE;

  // Camera connects are serialised, the following tracks the last attempt
  Camera *m_ConnectCamera = nullptr;
  esp_power_level_t m_Power = ESP_PWR_LVL_P3;
};

};  // namespace Furble

extern "C" {
void control_task(void *param);
}

struct FurbleCtx {
  Furble::Control *control;
  bool cancelled;
};

#endif
