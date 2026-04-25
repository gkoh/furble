#include <esp_bt.h>
#include <nvs_flash.h>

#include "FurbleSettings.h"
#include "FurbleTypes.h"
#include "Preferences.h"

namespace Furble {
Preferences Settings::m_Prefs;

const uint32_t Settings::BAUD_9600;

const std::unordered_map<Settings::type_t, Settings::setting_t> Settings::m_Setting = {
    {BRIGHTNESS,        {BRIGHTNESS, "Brightness", "brightness", "M5ez"}               },
    {INACTIVITY,        {INACTIVITY, "Inactivity", "inactivity", "M5ez"}               },
    {THEME,             {THEME, "Theme", "theme", "M5ez"}                              },
    {TX_POWER,          {TX_POWER, "TX Power", "tx_power", FURBLE_STR}                 },
    {GPS,               {GPS, "GPS", "gps", FURBLE_STR}                                },
    {GPS_BAUD,          {GPS_BAUD, "GPS Baud", "gps_baud", FURBLE_STR}                 },
    {INTERVAL,          {INTERVAL, "Interval", "interval", FURBLE_STR}                 },
    {MULTICONNECT,      {MULTICONNECT, "Multi-Connect", "multiconnect", FURBLE_STR}    },
    {RECONNECT,         {RECONNECT, "Infinite-ReConnect", "reconnect", FURBLE_STR}     },
    {FAUXNY,            {FAUXNY, "FauxNY", "fauxNY", FURBLE_STR}                       },
    {TOUCH_CALIBRATION, {TOUCH_CALIBRATION, "Touch Calibration", "t_calib", FURBLE_STR}},
    {AUTOCONNECT,       {AUTOCONNECT, "Auto-Connect", "autoconnect", FURBLE_STR}       },
};

const Settings::setting_t &Settings::get(type_t type) {
  return m_Setting.at(type);
}

template <>
bool Settings::load<bool>(type_t type) {
  const auto &setting = get(type);
  m_Prefs.begin(setting.nvs_namespace, true);
  bool value = m_Prefs.get<bool>(setting.key);
  m_Prefs.end();

  return value;
}

template <>
uint8_t Settings::load<uint8_t>(type_t type) {
  const auto &setting = get(type);
  m_Prefs.begin(setting.nvs_namespace, true);
  uint8_t value = m_Prefs.get<uint8_t>(setting.key);
  m_Prefs.end();

  return value;
}

template <>
uint32_t Settings::load<uint32_t>(type_t type) {
  const auto &setting = get(type);
  m_Prefs.begin(setting.nvs_namespace, true);
  uint32_t value = m_Prefs.get<uint32_t>(setting.key);
  m_Prefs.end();

  return value;
}

template <>
std::string Settings::load<std::string>(type_t type) {
  const auto &setting = get(type);
  m_Prefs.begin(setting.nvs_namespace, true);
  std::string value = m_Prefs.get(setting.key);
  m_Prefs.end();

  return value;
}

template <>
interval_t Settings::load<interval_t>(type_t type) {
  const auto &setting = get(type);
  interval_t interval;

  m_Prefs.begin(setting.nvs_namespace, true);
  size_t len = m_Prefs.get(setting.key, &interval, sizeof(interval_t));
  if (len == sizeof(interval_v1_t)) {
    // migrate v1 interval settings
    interval.wait = INTERVAL_DEFAULT_WAIT;
  } else if (len != sizeof(interval_t)) {
    // default values
    interval.count = INTERVAL_DEFAULT_COUNT;
    interval.delay = INTERVAL_DEFAULT_DELAY;
    interval.shutter = INTERVAL_DEFAULT_SHUTTER;
    interval.wait = INTERVAL_DEFAULT_WAIT;
  }

  m_Prefs.end();

  return interval;
}

template <>
esp_power_level_t Settings::load<esp_power_level_t>(type_t type) {
  const auto &setting = get(type);
  m_Prefs.begin(setting.nvs_namespace, true);
  uint8_t value = m_Prefs.get<uint8_t>(setting.key);
  m_Prefs.end();

  switch (value) {
    case 0:
      return ESP_PWR_LVL_P3;
    case 1:
      return ESP_PWR_LVL_P6;
    case 2:
      return ESP_PWR_LVL_P9;
  }
  return ESP_PWR_LVL_P3;
}

template <>
Settings::calibration_t Settings::load<Settings::calibration_t>(type_t type) {
  const auto &setting = get(type);
  calibration_t calibration;

  m_Prefs.begin(setting.nvs_namespace, true);
  size_t len = m_Prefs.get(setting.key, &calibration, sizeof(calibration_t));
  if (len != sizeof(calibration_t)) {
    // default values
    calibration.points[0] = 0;
    calibration.points[1] = 0;
    calibration.points[2] = 0;
    calibration.points[3] = 0;
    calibration.points[4] = 0;
    calibration.points[5] = 0;
    calibration.points[6] = 0;
    calibration.points[7] = 0;
    calibration.calibrated = false;
  }

  m_Prefs.end();

  return calibration;
}

template <>
void Settings::save<bool>(const type_t type, const bool &value) {
  const auto &setting = get(type);
  m_Prefs.begin(setting.nvs_namespace, false);
  m_Prefs.put<bool>(setting.key, value);
  m_Prefs.end();
}

template <>
void Settings::save<uint8_t>(const type_t type, const uint8_t &value) {
  const auto &setting = get(type);
  m_Prefs.begin(setting.nvs_namespace, false);
  m_Prefs.put<uint8_t>(setting.key, value);
  m_Prefs.end();
}

template <>
void Settings::save<uint32_t>(const type_t type, const uint32_t &value) {
  const auto &setting = get(type);
  m_Prefs.begin(setting.nvs_namespace, false);
  m_Prefs.put<uint32_t>(setting.key, value);
  m_Prefs.end();
}

template <>
void Settings::save<interval_t>(const type_t type, const interval_t &value) {
  const auto &setting = get(type);
  m_Prefs.begin(setting.nvs_namespace, false);
  m_Prefs.put(setting.key, &value, sizeof(value));
  m_Prefs.end();
}

template <>
void Settings::save<std::string>(const type_t type, const std::string &value) {
  const auto &setting = get(type);
  m_Prefs.begin(setting.nvs_namespace, false);
  m_Prefs.put(setting.key, value);
  m_Prefs.end();
}

template <>
void Settings::save<Settings::calibration_t>(const type_t type, const calibration_t &value) {
  const auto &setting = get(type);
  m_Prefs.begin(setting.nvs_namespace, false);
  m_Prefs.put(setting.key, &value, sizeof(value));
  m_Prefs.end();
}

void Settings::init(void) {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Set default values for all settings
  for (const auto &it : m_Setting) {
    auto &setting = it.second;
    m_Prefs.begin(setting.nvs_namespace, false);
    if (!m_Prefs.isKey(setting.key)) {
      switch (setting.type) {
        case BRIGHTNESS:
          save<uint8_t>(setting.type, 128);
          break;
        case INACTIVITY:
          save<uint8_t>(setting.type, 0);
          break;
        case THEME:
          save<std::string>(setting.type, "Default");
          break;
        case TX_POWER:
          save<uint8_t>(setting.type, 0);
          break;
        case INTERVAL:
        {
          interval_t interval = {
              INTERVAL_DEFAULT_COUNT,
              INTERVAL_DEFAULT_DELAY,
              INTERVAL_DEFAULT_SHUTTER,
              INTERVAL_DEFAULT_WAIT,
          };
          save<interval_t>(setting.type, interval);
        } break;
        case GPS:
        case MULTICONNECT:
        case RECONNECT:
        case FAUXNY:
        case AUTOCONNECT:
          save<bool>(setting.type, false);
          break;
        case GPS_BAUD:
          save<uint32_t>(setting.type, BAUD_9600);
          break;
        case TOUCH_CALIBRATION:
        {
          calibration_t calibration = {
              .points = {0x00},
              .calibrated = false,
          };
          save<calibration_t>(setting.type, calibration);
        } break;
      }
    }
    m_Prefs.end();
  }
}
}  // namespace Furble
