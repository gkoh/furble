#include <M5Unified.h>
#include <freertos/FreeRTOS.h>
#include <lvgl.h>

#include "Device.h"
#include "Scan.h"

#include "FurbleControl.h"
#include "FurbleSettings.h"
#include "FurbleUI.h"

extern "C" {

static void vUITask(void *param) {
  using namespace Furble;
  auto interval = Settings::load<interval_t>(Settings::INTERVAL);
  auto ui = UI(interval);

  ui.task();
}

void app_main() {
  BaseType_t xRet;
  TaskHandle_t xControlHandle = NULL;

  ESP_LOGI(LOG_TAG, "furble version: '%s'", FURBLE_VERSION);

  esp_pm_config_t pm_config = {
      .max_freq_mhz = 160,
      .min_freq_mhz = 40,
      .light_sleep_enable = true,
  };
  ESP_ERROR_CHECK(esp_pm_configure(&pm_config));

  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.internal_spk = false;
  cfg.internal_mic = false;
  M5.begin(cfg);

  Furble::Settings::init();
  Furble::Device::init(Furble::Settings::load<esp_power_level_t>(Furble::Settings::TX_POWER));

  auto &control = Furble::Control::getInstance();
  xRet = xTaskCreate(control_task, "control", 8192, &control, 4, &xControlHandle);
  if (xRet != pdPASS) {
    ESP_LOGE(LOG_TAG, "Failed to create control task.");
    abort();
  }

  // Run UI in host task (here)
  vUITask(NULL);
}
}
