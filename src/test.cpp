#include <Arduino.h>
#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>

#include "Device.h"
#include "Scan.h"

extern "C" {

void app_main() {
  // Serial.begin(115200);

  ESP_LOGI(LOG_TAG, "test");

  Furble::Device::init(ESP_PWR_LVL_P3);

  for (auto n = 30; n > 0; n--) {
    ESP_LOGI("test", "Countdown to scan ... %d", n);
    vTaskDelay(pdTICKS_TO_MS(1000));
  }

  auto &scan = Furble::Scan::getInstance();

  ESP_LOGI("test", "Scanning start");
  scan.start([](void *param) { ESP_LOGI("test", "Match!"); }, NULL);

  for (auto n = 15; n > 0; n--) {
    ESP_LOGI("test", "Scan countdown ... %d", n);
    vTaskDelay(pdTICKS_TO_MS(1000));
  }

  scan.stop();

  for (auto n = 0; n < Furble::CameraList::size(); n++) {
    auto *camera = Furble::CameraList::get(n);
    ESP_LOGI("test", "Found %s", camera->getName().c_str());
  }

  if (Furble::CameraList::size() > 0) {
    auto *camera = Furble::CameraList::get(0);

    ESP_LOGI("test", "Connecting to %s", camera->getName().c_str());
    if (camera->connect(ESP_PWR_LVL_P3, 10 * 120)) {
      ESP_LOGI("test", "Connected!");
    }
  }

  for (;;) {
    ESP_LOGI("test", "Waiting for infinity");
    vTaskDelay(pdTICKS_TO_MS(10000));
  }
}
}
