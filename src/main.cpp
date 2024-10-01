#include <Arduino.h>
#include <M5Unified.h>
#include <freertos/FreeRTOS.h>
#include <lvgl.h>

#include "nimconfig.h"

#include "Device.h"
#include "Furble.h"

#include "FurbleControl.h"
#include "FurbleSettings.h"
#include "FurbleUI.h"

void setup() {
  BaseType_t xRet;
  TaskHandle_t xControlHandle = NULL;
  TaskHandle_t xUIHandle = NULL;

  Serial.begin(115200);

  ESP_LOGI(LOG_TAG, "furble version: '%s'", FURBLE_VERSION);

  auto cfg = M5.config();
  cfg.internal_imu = false;
  cfg.internal_spk = false;
  cfg.internal_mic = false;
  M5.begin(cfg);

  Furble::Settings::init();
  Furble::Device::init();
  Furble::Scan::init(Furble::Settings::load<esp_power_level_t>(Furble::Settings::TX_POWER));

  auto &control = Furble::Control::getInstance();
  xRet = xTaskCreatePinnedToCore(control_task, "control", 8192, &control, 4, &xControlHandle, 1);
  if (xRet != pdPASS) {
    ESP_LOGE(LOG_TAG, "Failed to create control task.");
    abort();
  }

  // Pin UI to same core (0) as NimBLE
  xRet = xTaskCreatePinnedToCore(vUITask, "ui", 16384, &control, 2, &xUIHandle,
                                 CONFIG_BT_NIMBLE_PINNED_TO_CORE);
  if (xRet != pdPASS) {
    ESP_LOGE(LOG_TAG, "Failed to create UI task.");
    abort();
  }
}

void loop() {}
