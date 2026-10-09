#ifndef FURBLE_CONTROL_H
#define FURBLE_CONTROL_H

#include <atomic>
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
    STATE_ACTIVE,
    /** Disconnecting. */
    STATE_DISCONNECTING,
  } state_t;

  class Target {
    friend class Control;

   public:
    Target(Camera *camera,
           const std::atomic<uint32_t> *generation = nullptr,
           const std::atomic<uint32_t> *shutterReleaseRequests = nullptr,
           const std::atomic<uint32_t> *focusReleaseRequests = nullptr);
    ~Target();

    Camera *getCamera(void) const;
    cmd_t getCommand(void);
    BaseType_t sendCommand(cmd_t cmd);
    void updateGPS(const Camera::gps_t &gps, const Camera::timesync_t &timesync);

    void task(void);

   protected:
    std::atomic<bool> m_Stopped {false};

   private:
    BaseType_t sendCommand(cmd_t cmd, uint32_t generation);
    struct QueuedCommand {
      cmd_t command;
      uint32_t generation;
    };
    static constexpr UBaseType_t m_QueueLength = 8;

    QueueHandle_t m_Queue = NULL;
    Furble::Camera *m_Camera = NULL;
    Camera::gps_t m_GPS {};
    Camera::timesync_t m_Timesync {};
    std::mutex m_GPSMutex;
    std::atomic<bool> m_StopRequested {false};
    std::atomic<uint8_t> m_ReleaseRequested {0};
    const std::atomic<uint32_t> *m_Generation = nullptr;
    const std::atomic<uint32_t> *m_ShutterReleaseRequests = nullptr;
    const std::atomic<uint32_t> *m_FocusReleaseRequests = nullptr;
    uint32_t m_SeenShutterRelease = 0;
    uint32_t m_SeenFocusRelease = 0;
    uint32_t m_CommandGeneration = 0;
  };

  static Control &getInstance();

  Control(Control const &) = delete;
  Control(Control &&) = delete;
  Control &operator=(Control const &) = delete;
  Control &operator=(Control &&) = delete;

  const uint32_t TIMEOUT_DEFAULT_MS = (30 * 1000);
  const uint32_t TIMEOUT_INFINITE_MS = (5 * 1000);
  const uint32_t SLEEP_INFINITE_MS = (5 * 1000);

  /**
   * FreeRTOS control task function.
   */
  void task(void);

  /**
   * Send a command to active connections. pdTRUE means the command was
   * accepted or its release was recorded for retry, not acknowledged by BLE.
   * Releases are also accepted while reconnecting so a still-connected camera
   * can drop a held shutter or focus button.
   */
  BaseType_t sendCommand(cmd_t cmd);

  /**
   * Update GPS and timesync values.
   */
  BaseType_t updateGPS(const Camera::gps_t &gps, const Camera::timesync_t &timesync);

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
  bool connectAll(bool infiniteReconnect);

  /**
   * Disconnect all connected cameras.
   */
  void disconnect(void);

  /**
   * Add specified camera to active target list.
   */
  bool addActive(Camera *camera);

  /**
   * Get current camera connection attempt.
   *
   * @return Camera being connected otherwise nullptr.
   */
  Camera *getConnectingCamera(void) const;

  /** Retrieve current control state. */
  state_t getState(void) const;

  struct ConnectionStatus {
    state_t state;
    std::string connectingName;
    uint8_t progress;
    uint32_t commandFailures;
  };
  /** Snapshot status and the cumulative number of rejected press commands. */
  ConnectionStatus getConnectionStatus(void) const;

  /** Set transmit power. */
  void setPower(esp_power_level_t power);

 private:
  Control();

  struct QueuedCommand {
    cmd_t command;
    uint32_t generation;
  };

  bool allConnectedLocked(void);

  /** Iterate over cameras and attempt connection. */
  state_t connectAll(void);

  static constexpr UBaseType_t m_QueueLength = 32;

  QueueHandle_t m_Queue = NULL;
  std::mutex m_Mutex;
  std::vector<std::unique_ptr<Control::Target>> m_Targets;

  std::atomic<bool> m_InfiniteReconnect {false};
  std::atomic<state_t> m_State {STATE_IDLE};
  std::atomic<uint32_t> m_Generation {0};
  std::atomic<uint8_t> m_PendingReleases {0};
  std::atomic<uint32_t> m_ShutterReleaseRequests {0};
  std::atomic<uint32_t> m_FocusReleaseRequests {0};
  std::atomic<bool> m_LastShutterWasRelease {false};
  std::atomic<bool> m_LastFocusWasRelease {false};
  std::atomic<uint32_t> m_CommandFailures {0};
  mutable std::mutex m_StatusMutex;
  uint32_t m_FailCount = 0;

  // Camera connects are serialised, the following tracks the last attempt
  std::atomic<Camera *> m_ConnectCamera {nullptr};
  std::atomic<esp_power_level_t> m_Power {ESP_PWR_LVL_P3};
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
