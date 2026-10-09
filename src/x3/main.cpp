#include <cstdlib>

#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Device.h"
#include "FurbleControl.h"
#include "FurbleSettings.h"
#include "Scan.h"
#include "x3/FurbleX3App.h"
#include "x3/FurbleX3Platform.h"

extern "C" void app_main() {
  ESP_LOGI(LOG_TAG, "furble X3 version: '%s'", FURBLE_VERSION);
  const esp_err_t init = Furble::X3Platform::init();
  if (init != ESP_OK) {
    ESP_LOGE(LOG_TAG, "X3 initialization failed: %s", esp_err_to_name(init));
    Furble::X3Platform::startupFailure();
  }
  Furble::Settings::init();
  Furble::Device::init(Furble::Settings::load<esp_power_level_t>(Furble::Settings::TX_POWER));

  auto &control = Furble::Control::getInstance();
  if (xTaskCreate(control_task, "control", 8192, &control, 4, nullptr) != pdPASS) {
    ESP_LOGE(LOG_TAG, "Failed to create X3 control task");
    Furble::X3Platform::startupFailure();
  }
  // Lifecycle work and e-paper rendering run in their own tasks. The app_main
  // task keeps polling keys and advancing interval timing every 10 ms.
  Furble::X3App app(Furble::X3Platform::hooks());
  app.task();
}
