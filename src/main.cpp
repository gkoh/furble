#include <Arduino.h>
#include <freertos/FreeRTOS.h>

#include "nimconfig.h"

#include "Device.h"
#include "Furble.h"

#include "furble_control.h"
#include "furble_ui.h"
#include "settings.h"

void setup() {
  BaseType_t xRet;
  TaskHandle_t xControlHandle = NULL;
  TaskHandle_t xUIHandle = NULL;

  Serial.begin(115200);

  Furble::Device::init();
  Furble::Scan::init(settings_load_esp_tx_power());

  Furble::Control *control = new Furble::Control();

  xRet = xTaskCreatePinnedToCore(control_task, "control", 8192, control, 4, &xControlHandle, 1);
  if (xRet != pdPASS) {
    ESP_LOGE(LOG_TAG, "Failed to create control task.");
    abort();
  }

  xRet = xTaskCreatePinnedToCore(vUITask, "UI-M5ez", 32768, control, 2, &xUIHandle, 1);
  if (xRet != pdPASS) {
    ESP_LOGE(LOG_TAG, "Failed to create UI task.");
    abort();
  }
}

void loop() {}
