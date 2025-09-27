#include <Arduino.h>
#include <M5Unified.h>
#include <freertos/FreeRTOS.h>
#include <lvgl.h>

#include "nimconfig.h"

#include "Device.h"
#include "Scan.h"

#include "FurbleControl.h"
#include "FurbleSettings.h"
#include "FurbleUI.h"

extern "C" {

void app_main() {
  BaseType_t xRet;
  TaskHandle_t xControlHandle = NULL;

  Serial.begin(115200);

  ESP_LOGI(LOG_TAG, "furble version: '%s'", FURBLE_VERSION);

  esp_pm_config_esp32_t pm_config = {
      .max_freq_mhz = 80,
      .min_freq_mhz = 80,
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
  xRet = xTaskCreatePinnedToCore(control_task, "control", 8192, &control, 4, &xControlHandle, 1);
  if (xRet != pdPASS) {
    ESP_LOGE(LOG_TAG, "Failed to create control task.");
    abort();
  }

  // Run UI in host task (here)
  vUITask(NULL);
}
}
