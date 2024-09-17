#include <CameraList.h>

#include "furble_control.h"

static void target_task(void *param) {
  auto *target = static_cast<Furble::Control::Target *>(param);

  Furble::Camera *camera = target->getCamera();
  const char *name = camera->getName().c_str();

  while (true) {
    control_cmd_t cmd = target->getCommand();
    switch (cmd) {
      case CONTROL_CMD_SHUTTER_PRESS:
        ESP_LOGI(LOG_TAG, "shutterPress(%s)", name);
        camera->shutterPress();
        break;
      case CONTROL_CMD_SHUTTER_RELEASE:
        ESP_LOGI(LOG_TAG, "shutterRelease(%s)", name);
        camera->shutterRelease();
        break;
      case CONTROL_CMD_FOCUS_PRESS:
        ESP_LOGI(LOG_TAG, "focusPress(%s)", name);
        camera->focusPress();
        break;
      case CONTROL_CMD_FOCUS_RELEASE:
        ESP_LOGI(LOG_TAG, "focusRelease(%s)", name);
        camera->focusRelease();
        break;
      case CONTROL_CMD_GPS_UPDATE:
        ESP_LOGI(LOG_TAG, "updateGeoData(%s)", name);
        camera->updateGeoData(target->getGPS(), target->getTimesync());
        break;
      case CONTROL_CMD_ERROR:
        // ignore continue
        break;
      case CONTROL_CMD_DISCONNECT:
        goto task_exit;
      default:
        ESP_LOGE(LOG_TAG, "Invalid control command %d.", cmd);
    }
  }
task_exit:
  vTaskDelete(NULL);
}

namespace Furble {
Control::Target::Target(Camera *camera) {
  m_Camera = camera;
  m_Queue = xQueueCreate(TARGET_CMD_QUEUE_LEN, sizeof(control_cmd_t));
}

Control::Target::~Target() {
  vQueueDelete(m_Queue);
  m_Queue = NULL;
  m_Camera->disconnect();
  m_Camera = NULL;
}

Camera *Control::Target::getCamera(void) {
  return m_Camera;
}

void Control::Target::sendCommand(control_cmd_t cmd) {
  BaseType_t ret = xQueueSend(m_Queue, &cmd, 0);
  if (ret != pdTRUE) {
    ESP_LOGE(LOG_TAG, "Failed to send command to target.");
  }
}

control_cmd_t Control::Target::getCommand(void) {
  control_cmd_t cmd = CONTROL_CMD_ERROR;
  BaseType_t ret = xQueueReceive(m_Queue, &cmd, pdMS_TO_TICKS(50));
  if (ret != pdTRUE) {
    return CONTROL_CMD_ERROR;
  }
  return cmd;
}

const Camera::gps_t &Control::Target::getGPS(void) {
  return m_GPS;
}

const Camera::timesync_t &Control::Target::getTimesync(void) {
  return m_Timesync;
}

void Control::Target::updateGPS(Camera::gps_t &gps, Camera::timesync_t &timesync) {
  m_GPS = gps;
  m_Timesync = timesync;
}

Control::Control(void) {
  m_Queue = xQueueCreate(CONTROL_CMD_QUEUE_LEN, sizeof(control_cmd_t));
  if (m_Queue == NULL) {
    ESP_LOGE(LOG_TAG, "Failed to create control queue.");
    abort();
  }
}

Control::~Control() {
  vQueueDelete(m_Queue);
  m_Queue = NULL;
}

void Control::task(void) {
  while (true) {
    control_cmd_t cmd;
    BaseType_t ret = xQueueReceive(m_Queue, &cmd, pdMS_TO_TICKS(50));
    if (ret == pdTRUE) {
      for (const auto &target : m_Targets) {
        switch (cmd) {
          case CONTROL_CMD_SHUTTER_PRESS:
          case CONTROL_CMD_SHUTTER_RELEASE:
          case CONTROL_CMD_FOCUS_PRESS:
          case CONTROL_CMD_FOCUS_RELEASE:
          case CONTROL_CMD_GPS_UPDATE:
            target->sendCommand(cmd);
            break;
          default:
            ESP_LOGE(LOG_TAG, "Invalid control command %d.", cmd);
            break;
        }
      }
    }
  }
}

BaseType_t Control::sendCommand(control_cmd_t cmd) {
  return xQueueSend(m_Queue, &cmd, 0);
}

BaseType_t Control::updateGPS(Camera::gps_t &gps, Camera::timesync_t &timesync) {
  for (const auto &target : m_Targets) {
    target->updateGPS(gps, timesync);
  }

  control_cmd_t cmd = CONTROL_CMD_GPS_UPDATE;
  return xQueueSend(m_Queue, &cmd, 0);
}

bool Control::isConnected(void) {
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

void Control::disconnect(void) {
  for (const auto &target : m_Targets) {
    target->sendCommand(CONTROL_CMD_DISCONNECT);
  }

  m_Targets.clear();
}

void Control::addActive(Camera *camera) {
  auto target = std::unique_ptr<Control::Target>(new Control::Target(camera));

  // Create per-target task that will self-delete on disconnect
  BaseType_t ret = xTaskCreatePinnedToCore(target_task, camera->getName().c_str(), 4096,
                                           target.get(), 3, NULL, 1);
  if (ret != pdPASS) {
    ESP_LOGE(LOG_TAG, "Failed to create task for '%s'.", camera->getName());
  } else {
    m_Targets.push_back(std::move(target));
  }
}

};  // namespace Furble

void control_task(void *param) {
  Furble::Control *control = static_cast<Furble::Control *>(param);

  control->task();
}
