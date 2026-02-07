#include <esp_pm.h>

#include "powermgmt.h"

void powermgmt_sleep(bool enable) {
  esp_pm_config_t pm_config = {
      .max_freq_mhz = POWERMGMT_MAX_FREQ_MHZ,
      .min_freq_mhz = POWERMGMT_MIN_FREQ_MHZ,
      .light_sleep_enable = enable,
  };
  ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
}
