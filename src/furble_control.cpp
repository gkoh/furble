#include <CameraList.h>

#include "furble_control.h"

static QueueHandle_t queue;

static Furble::Camera::gps_t last_gps;
static Furble::Camera::timesync_t last_timesync;

void control_update_gps(Furble::Camera::gps_t &gps, Furble::Camera::timesync_t &timesync) {
  last_gps = gps;
  last_timesync = timesync;

  control_cmd_t cmd = CONTROL_CMD_GPS_UPDATE;
  xQueueSend(queue, &cmd, sizeof(control_cmd_t));
}

void control_task(void *param) {
  queue = static_cast<QueueHandle_t>(param);

  while (true) {
    control_cmd_t cmd;
    BaseType_t ret = xQueueReceive(queue, &cmd, pdMS_TO_TICKS(50));
    if (ret == pdTRUE) {
      for (size_t n = 0; n < Furble::CameraList::size(); n++) {
        Furble::Camera *camera = Furble::CameraList::get(n);
        if (!camera->isActive()) {
          continue;
        }

        switch (cmd) {
          case CONTROL_CMD_SHUTTER_PRESS:
            camera->shutterPress();
            ESP_LOGI(LOG_TAG, "shutterPress()");
            break;
          case CONTROL_CMD_SHUTTER_RELEASE:
            ESP_LOGI(LOG_TAG, "shutterRelease()");
            camera->shutterRelease();
            break;
          case CONTROL_CMD_FOCUS_PRESS:
            ESP_LOGI(LOG_TAG, "focusPress()");
            camera->focusPress();
            break;
          case CONTROL_CMD_FOCUS_RELEASE:
            ESP_LOGI(LOG_TAG, "focusRelease()");
            camera->focusRelease();
            break;
          case CONTROL_CMD_GPS_UPDATE:
            ESP_LOGI(LOG_TAG, "updateGeoData()");
            camera->updateGeoData(last_gps, last_timesync);
            break;
          default:
            ESP_LOGE(LOG_TAG, "Invalid control command %d.", cmd);
        }
      }
    }
  }
}
