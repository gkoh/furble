#include "FurbleControl.h"
#include "FurbleFatal.h"

#include <cstdlib>

namespace Furble {
[[noreturn]] __attribute__((weak)) void fatal() {
  std::abort();
}
}  // namespace Furble

namespace Furble {
Control::Target::Target(Camera *camera,
                        const std::atomic<uint32_t> *generation,
                        const std::atomic<uint32_t> *shutterReleaseRequests,
                        const std::atomic<uint32_t> *focusReleaseRequests)
    : m_Generation(generation),
      m_ShutterReleaseRequests(shutterReleaseRequests),
      m_FocusReleaseRequests(focusReleaseRequests),
      m_SeenShutterRelease(shutterReleaseRequests ? shutterReleaseRequests->load() : 0),
      m_SeenFocusRelease(focusReleaseRequests ? focusReleaseRequests->load() : 0) {
  m_Camera = camera;
  m_Queue = xQueueCreate(m_QueueLength, sizeof(QueuedCommand));
}

Control::Target::~Target() {
  if (m_Queue != NULL) {
    vQueueDelete(m_Queue);
  }
  m_Queue = NULL;
  m_Camera->disconnect();
  m_Camera = NULL;
}

Camera *Control::Target::getCamera(void) const {
  return m_Camera;
}

BaseType_t Control::Target::sendCommand(cmd_t cmd) {
  return sendCommand(cmd, m_Generation ? m_Generation->load() : 0);
}

BaseType_t Control::Target::sendCommand(cmd_t cmd, uint32_t generation) {
  if (cmd == CMD_DISCONNECT) {
    m_StopRequested.store(true);
    return pdTRUE;
  }
  // Drain the finite backlog before a fallback release. Coalesce additional
  // releases and reject new traffic so GPS/presses cannot starve recovery.
  if (m_ReleaseRequested.load() != 0) {
    const uint8_t release = cmd == CMD_SHUTTER_RELEASE ? 1 : cmd == CMD_FOCUS_RELEASE ? 2 : 0;
    if (release != 0)
      m_ReleaseRequested.fetch_or(release);
    return release != 0 ? pdTRUE : pdFALSE;
  }
  QueuedCommand queued {cmd, generation};
  BaseType_t ret = xQueueSend(m_Queue, &queued, 0);
  if (ret != pdTRUE) {
    if (cmd == CMD_SHUTTER_RELEASE) {
      m_ReleaseRequested.fetch_or(1);
      ret = pdTRUE;
    }
    if (cmd == CMD_FOCUS_RELEASE) {
      m_ReleaseRequested.fetch_or(2);
      ret = pdTRUE;
    }
    if (ret != pdTRUE)
      ESP_LOGE(LOG_TAG, "Failed to send command to target.");
  }
  return ret;
}

Control::cmd_t Control::Target::getCommand(void) {
  QueuedCommand queued {CMD_ERROR, 0};
  BaseType_t ret = xQueueReceive(m_Queue, &queued, pdMS_TO_TICKS(50));
  if (ret != pdTRUE) {
    return CMD_ERROR;
  }
  m_CommandGeneration = queued.generation;
  return queued.command;
}

void Control::Target::updateGPS(const Camera::gps_t &gps, const Camera::timesync_t &timesync) {
  const std::lock_guard<std::mutex> lock(m_GPSMutex);
  m_GPS = gps;
  m_Timesync = timesync;
}

void Control::Target::task(void) {
  const char *name = m_Camera->getName().c_str();
  bool shutterHeld = false;
  bool focusHeld = false;

  auto release = [&](uint8_t buttons) {
    if ((buttons & 1) && shutterHeld) {
      if (m_Camera->isConnected())
        m_Camera->shutterRelease();
      shutterHeld = false;
    }
    if ((buttons & 2) && focusHeld) {
      if (m_Camera->isConnected())
        m_Camera->focusRelease();
      focusHeld = false;
    }
  };

  while (true) {
    cmd_t cmd = this->getCommand();
    if (m_StopRequested.load()) {
      release(3);
      m_Camera->setActive(false);
      break;
    }
    const uint32_t generation = m_Generation ? m_Generation->load() : 0;
    if (!m_Camera->isConnected()) {
      shutterHeld = false;
      focusHeld = false;
      m_ReleaseRequested.exchange(0);
      continue;
    }
    uint8_t requested = 0;
    if (m_ShutterReleaseRequests) {
      uint32_t current = m_ShutterReleaseRequests->load();
      if (current != m_SeenShutterRelease) {
        m_SeenShutterRelease = current;
        requested |= 1;
      }
    }
    if (m_FocusReleaseRequests) {
      uint32_t current = m_FocusReleaseRequests->load();
      if (current != m_SeenFocusRelease) {
        m_SeenFocusRelease = current;
        requested |= 2;
      }
    }
    release(requested);
    if (cmd != CMD_ERROR && m_CommandGeneration != generation)
      continue;
    // Drain older commands before applying a release that could not be queued.
    if (cmd == CMD_ERROR)
      release(m_ReleaseRequested.exchange(0));
    switch (cmd) {
      case CMD_SHUTTER_PRESS:
        ESP_LOGI(LOG_TAG, "shutterPress(%s)", name);
        m_Camera->shutterPress();
        shutterHeld = true;
        break;
      case CMD_SHUTTER_RELEASE:
        ESP_LOGI(LOG_TAG, "shutterRelease(%s)", name);
        m_Camera->shutterRelease();
        shutterHeld = false;
        break;
      case CMD_FOCUS_PRESS:
        ESP_LOGI(LOG_TAG, "focusPress(%s)", name);
        m_Camera->focusPress();
        focusHeld = true;
        break;
      case CMD_FOCUS_RELEASE:
        ESP_LOGI(LOG_TAG, "focusRelease(%s)", name);
        m_Camera->focusRelease();
        focusHeld = false;
        break;
      case CMD_GPS_UPDATE:
      {
        Camera::gps_t gps;
        Camera::timesync_t timesync;
        {
          const std::lock_guard<std::mutex> lock(m_GPSMutex);
          gps = m_GPS;
          timesync = m_Timesync;
        }
        ESP_LOGI(LOG_TAG, "updateGeoData(%s)", name);
        m_Camera->updateGeoData(gps, timesync);
        break;
      }
      case CMD_DISCONNECT:
        release(3);
        m_Camera->setActive(false);
        goto task_exit;
      case CMD_ERROR:
        // ignore continue
        break;
      default:
        ESP_LOGE(LOG_TAG, "Invalid control command %d.", cmd);
    }
  }
task_exit:
  m_Stopped = true;
  vTaskDelete(NULL);
}

Control &Control::getInstance(void) {
  static Control instance;
  return instance;
}

Control::Control() {
  m_Queue = xQueueCreate(m_QueueLength, sizeof(QueuedCommand));
  if (m_Queue == NULL) {
    ESP_LOGE(LOG_TAG, "Failed to create control queue.");
    fatal();
  }
}

Control::state_t Control::connectAll(void) {
  uint32_t timeout = m_InfiniteReconnect.load() ? TIMEOUT_INFINITE_MS : TIMEOUT_DEFAULT_MS;
  const std::lock_guard<std::mutex> lock(m_Mutex);

  if (m_Targets.empty())
    return STATE_CONNECT_FAILED;

  // Iterate over cameras and attempt connection.
  Camera *camera = nullptr;
  for (const auto &target : m_Targets) {
    if (m_State.load() == STATE_DISCONNECTING) {
      return STATE_DISCONNECTING;
    }
    camera = target->getCamera();
    if (!camera->isConnected()) {
      {
        const std::lock_guard<std::mutex> statusLock(m_StatusMutex);
        m_ConnectCamera = camera;
      }
      if (!camera->connect(m_Power.load(), timeout)) {
        m_FailCount++;
        break;
      } else {
        const std::lock_guard<std::mutex> statusLock(m_StatusMutex);
        m_ConnectCamera = nullptr;
      }
    }
  }

  if (m_State.load() == STATE_DISCONNECTING) {
    return STATE_DISCONNECTING;
  }

  if (allConnectedLocked()) {
    m_FailCount = 0;
    return STATE_ACTIVE;
  }

  if (m_InfiniteReconnect.load() || (m_FailCount < 2)) {
    if (m_InfiniteReconnect.load()) {
      // sleep to idle
      vTaskDelay(pdMS_TO_TICKS(SLEEP_INFINITE_MS));
    }
    return STATE_CONNECT;
  }

  return STATE_CONNECT_FAILED;
}

void Control::task(void) {
  while (true) {
    QueuedCommand queued {};
    BaseType_t ret = xQueueReceive(m_Queue, &queued, pdMS_TO_TICKS(50));
    cmd_t cmd = queued.command;
    if (ret == pdTRUE && queued.generation != m_Generation.load()) {
      ret = pdFALSE;
    }

    switch (m_State.load()) {
      case STATE_IDLE:
      {
        const std::lock_guard<std::mutex> lock(m_Mutex);
        // disconnect() may have advanced the generation while we waited.
        if (queued.generation != m_Generation.load())
          break;
        if (ret == pdTRUE) {
          if (cmd == CMD_CONNECT) {
            state_t expected = STATE_IDLE;
            m_State.compare_exchange_strong(expected, STATE_CONNECT);
            continue;
          }
        }
        break;
      }

      case STATE_CONNECT:
      {
        state_t expected = STATE_CONNECT;
        if (!m_State.compare_exchange_strong(expected, STATE_CONNECTING)) {
          break;
        }
        state_t next = connectAll();
        expected = STATE_CONNECTING;
        m_State.compare_exchange_strong(expected, next);
        break;
      }

      case STATE_CONNECTING:
      case STATE_CONNECT_FAILED:
        break;

      case STATE_ACTIVE:
      {
        const std::lock_guard<std::mutex> lock(m_Mutex);
        if (m_State.load() != STATE_ACTIVE) {
          break;
        }
        if (!allConnectedLocked()) {
          // Commands from the lost connection must not replay after reconnect.
          state_t expected = STATE_ACTIVE;
          if (!m_State.compare_exchange_strong(expected, STATE_CONNECT))
            break;
          m_Generation.fetch_add(1);
          xQueueReset(m_Queue);
          for (const auto &target : m_Targets) {
            xQueueReset(target->m_Queue);
          }
          // A release accepted before the drop may have been in either queue.
          // The latest accepted button intent decides which releases survive.
          if (m_LastShutterWasRelease.load())
            m_ShutterReleaseRequests.fetch_add(1);
          if (m_LastFocusWasRelease.load())
            m_FocusReleaseRequests.fetch_add(1);
          continue;
        }

        // Fallback releases must follow all commands already accepted.
        if (ret != pdTRUE) {
          uint8_t releases = m_PendingReleases.exchange(0);
          for (const auto &target : m_Targets) {
            if (releases & 1)
              target->sendCommand(CMD_SHUTTER_RELEASE);
            if (releases & 2)
              target->sendCommand(CMD_FOCUS_RELEASE);
          }
        }
        if (queued.generation != m_Generation.load()) {
          ret = pdFALSE;
        }
        if (ret == pdTRUE) {
          for (const auto &target : m_Targets) {
            switch (cmd) {
              case CMD_SHUTTER_PRESS:
              case CMD_SHUTTER_RELEASE:
              case CMD_FOCUS_PRESS:
              case CMD_FOCUS_RELEASE:
              case CMD_GPS_UPDATE:
                if (target->sendCommand(cmd, queued.generation) != pdTRUE
                    && cmd != CMD_GPS_UPDATE) {
                  m_CommandFailures.fetch_add(1);
                }
                break;
              default:
                ESP_LOGE(LOG_TAG, "Invalid control command %d.", cmd);
                break;
            }
          }
        }
        break;
      }

      case STATE_DISCONNECTING:
        break;
    }
  }
}

BaseType_t Control::sendCommand(cmd_t cmd) {
  QueuedCommand queued {cmd, m_Generation.load()};
  if (cmd == CMD_CONNECT && m_State.load() != STATE_IDLE)
    return pdFALSE;
  if (cmd != CMD_CONNECT && m_State.load() != STATE_ACTIVE) {
    if (cmd == CMD_SHUTTER_RELEASE) {
      m_ShutterReleaseRequests.fetch_add(1);
      return pdTRUE;
    }
    if (cmd == CMD_FOCUS_RELEASE) {
      m_FocusReleaseRequests.fetch_add(1);
      return pdTRUE;
    }
    if (cmd == CMD_SHUTTER_PRESS || cmd == CMD_FOCUS_PRESS)
      m_CommandFailures.fetch_add(1);
    return pdFALSE;
  }
  BaseType_t ret = pdFALSE;
  if (m_PendingReleases.load() != 0) {
    const uint8_t release = cmd == CMD_SHUTTER_RELEASE ? 1 : cmd == CMD_FOCUS_RELEASE ? 2 : 0;
    if (release != 0)
      m_PendingReleases.fetch_or(release);
    if (release == 0 && (cmd == CMD_SHUTTER_PRESS || cmd == CMD_FOCUS_PRESS))
      m_CommandFailures.fetch_add(1);
    ret = release != 0 ? pdTRUE : pdFALSE;
  } else {
    ret = xQueueSend(m_Queue, &queued, 0);
    if (ret != pdTRUE) {
      if (cmd == CMD_SHUTTER_PRESS || cmd == CMD_FOCUS_PRESS)
        m_CommandFailures.fetch_add(1);
      if (cmd == CMD_SHUTTER_RELEASE) {
        m_PendingReleases.fetch_or(1);
        ret = pdTRUE;
      }
      if (cmd == CMD_FOCUS_RELEASE) {
        m_PendingReleases.fetch_or(2);
        ret = pdTRUE;
      }
    }
  }
  if (ret == pdTRUE) {
    if (cmd == CMD_SHUTTER_PRESS)
      m_LastShutterWasRelease.store(false);
    if (cmd == CMD_FOCUS_PRESS)
      m_LastFocusWasRelease.store(false);
    if (cmd == CMD_SHUTTER_RELEASE) {
      m_LastShutterWasRelease.store(true);
      if (m_State.load() != STATE_ACTIVE || m_Generation.load() != queued.generation)
        m_ShutterReleaseRequests.fetch_add(1);
    }
    if (cmd == CMD_FOCUS_RELEASE) {
      m_LastFocusWasRelease.store(true);
      if (m_State.load() != STATE_ACTIVE || m_Generation.load() != queued.generation)
        m_FocusReleaseRequests.fetch_add(1);
    }
  }
  return ret;
}

BaseType_t Control::updateGPS(const Camera::gps_t &gps, const Camera::timesync_t &timesync) {
  // GPS runs on the UI task. Never wait behind a connect, retry delay or
  // disconnect; the next GPS update will supply fresh data.
  std::unique_lock<std::mutex> lock(m_Mutex, std::try_to_lock);
  if (!lock.owns_lock() || m_State.load() != STATE_ACTIVE)
    return pdFALSE;
  for (const auto &target : m_Targets) {
    target->updateGPS(gps, timesync);
  }

  return sendCommand(CMD_GPS_UPDATE);
}

bool Control::allConnected(void) {
  const std::lock_guard<std::mutex> lock(m_Mutex);
  return allConnectedLocked();
}

bool Control::allConnectedLocked(void) {
  if (m_Targets.empty())
    return false;
  for (const auto &target : m_Targets) {
    if (!target->getCamera()->isConnected()) {
      return false;
    }
  }

  return true;
}

const std::vector<std::unique_ptr<Control::Target>> &Control::getTargets(void) {
  return m_Targets;
}

bool Control::connectAll(bool infiniteReconnect) {
  const std::lock_guard<std::mutex> lock(m_Mutex);
  if (m_State.load() != STATE_IDLE)
    return false;
  m_InfiniteReconnect = infiniteReconnect;
  m_FailCount = 0;

  if (this->sendCommand(CMD_CONNECT) != pdTRUE) {
    state_t expected = STATE_IDLE;
    m_State.compare_exchange_strong(expected, STATE_CONNECT_FAILED);
    return false;
  }
  return true;
}

void Control::disconnect(void) {
  m_State = STATE_DISCONNECTING;
  m_Generation.fetch_add(1);

  // Force cancel any active connection attempts
  ble_gap_conn_cancel();

  const std::lock_guard<std::mutex> lock(m_Mutex);

  // send disconnect
  for (const auto &target : m_Targets) {
    target->sendCommand(CMD_DISCONNECT);
  }

  // wait for tasks to finish
  for (const auto &target : m_Targets) {
    do {
      vTaskDelay(pdMS_TO_TICKS(1));
    } while (!target.get()->m_Stopped.load());
  }

  {
    const std::lock_guard<std::mutex> statusLock(m_StatusMutex);
    m_ConnectCamera = nullptr;
  }
  m_Targets.clear();
  xQueueReset(m_Queue);
  m_PendingReleases = 0;
  m_LastShutterWasRelease = false;
  m_LastFocusWasRelease = false;
  m_State = STATE_IDLE;
}

bool Control::addActive(Camera *camera) {
  const std::lock_guard<std::mutex> lock(m_Mutex);
  if (camera == nullptr || m_State.load() != STATE_IDLE)
    return false;

  auto target = std::make_unique<Control::Target>(camera, &m_Generation, &m_ShutterReleaseRequests,
                                                  &m_FocusReleaseRequests);
  if (target->m_Queue == NULL) {
    ESP_LOGE(LOG_TAG, "Failed to create queue for '%s'.", camera->getName().c_str());
    return false;
  }

  // Create per-target task that will self-delete on disconnect
  BaseType_t ret = xTaskCreate(
      [](void *param) {
        auto *target = static_cast<Furble::Control::Target *>(param);
        target->task();
      },
      camera->getName().c_str(), 4096, target.get(), 3, NULL);
  if (ret != pdPASS) {
    ESP_LOGE(LOG_TAG, "Failed to create task for '%s'.", camera->getName().c_str());
  } else {
    m_Targets.push_back(std::move(target));
    return true;
  }
  return false;
}

Camera *Control::getConnectingCamera(void) const {
  return m_ConnectCamera.load();
}

Control::state_t Control::getState(void) const {
  return m_State.load();
}

Control::ConnectionStatus Control::getConnectionStatus(void) const {
  const std::lock_guard<std::mutex> lock(m_StatusMutex);
  ConnectionStatus status {m_State.load(), {}, 0, m_CommandFailures.load()};
  Camera *camera = m_ConnectCamera.load();
  if (camera != nullptr) {
    status.connectingName = camera->getName();
    status.progress = camera->getConnectProgress();
  }
  return status;
}

void Control::setPower(esp_power_level_t power) {
  m_Power = power;
}

};  // namespace Furble

void control_task(void *param) {
  Furble::Control *control = static_cast<Furble::Control *>(param);

  control->task();
}
