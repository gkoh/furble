#include <cstddef>
#include <cstdint>

#include <Furble.h>
#include <Preferences.h>
#include <TinyGPS++.h>
#include <esp_bt.h>

#include "furble_gps.h"
#include "interval.h"
#include "settings.h"

const char *PREFS_TX_POWER = "txpower";
const char *PREFS_GPS = "gps";
const char *PREFS_INTERVAL = "interval";

/**
 * Global intervalometer configuration.
 */
interval_t interval;

/**
 * Save BLE transmit power to preferences.
 */
static void save_tx_power(uint8_t tx_power) {
  Preferences prefs;

  prefs.begin(FURBLE_STR, false);
  prefs.putUChar(PREFS_TX_POWER, tx_power);
  prefs.end();
}

/**
 * Load BLE transmit power from preferences.
 */
static uint8_t load_tx_power() {
  Preferences prefs;

  prefs.begin(FURBLE_STR, true);
  uint8_t power = prefs.getUChar(PREFS_TX_POWER, 1);
  prefs.end();

  return power;
}

/**
 * Load and map prefs power to ESP power.
 */
esp_power_level_t settings_load_esp_tx_power() {
  uint8_t power = load_tx_power();
  switch (power) {
    case 0:
      return ESP_PWR_LVL_P3;
    case 1:
      return ESP_PWR_LVL_P6;
    case 2:
      return ESP_PWR_LVL_P9;
    default:
      return ESP_PWR_LVL_P3;
  }
  return ESP_PWR_LVL_P3;
}

/**
 * Transmit power menu.
 *
 * Options are 1, 2 and 3.
 */
void settings_menu_tx_power(void) {
  uint8_t power = load_tx_power();
  ezProgressBar power_bar(FURBLE_STR, {"Set transmit power"}, {"Adjust", "Back"});
  power_bar.value(power / 0.03f);
  while (true) {
    std::string b = ez.buttons.poll();
    if (b == "Adjust") {
      power++;
      if (power > 3) {
        power = 1;
      }
      power_bar.value(power / 0.03f);
    }
    if (b == "Back") {
      break;
    }
  }

  save_tx_power(power);
}

/**
 * Display GPS data.
 */
static void show_gps_info(void) {
  Serial.println("GPS Data");
  char buffer0[64] = {0x0};
  char buffer1[64] = {0x0};
  char buffer2[64] = {0x0};
  char buffer3[64] = {0x0};
  bool first = true;

  do {
    if (M5.BtnB.wasReleased()) {
      break;
    }

    bool updated = furble_gps.location.isUpdated() || furble_gps.date.isUpdated()
                   || furble_gps.time.isUpdated();

    snprintf(buffer0, 64, "%s (%d)",
             furble_gps.location.isValid() && furble_gps.date.isValid() && furble_gps.time.isValid()
                 ? "Valid"
                 : "Invalid",
             furble_gps.location.age());
    snprintf(buffer1, 64, "%.2f %.2f", furble_gps.location.lat(), furble_gps.location.lng());
    snprintf(buffer2, 64, "%.2f metres", furble_gps.altitude.meters());
    snprintf(buffer3, 64, "%4u-%02u-%02u %02u:%02u:%02u", furble_gps.date.year(),
             furble_gps.date.month(), furble_gps.date.day(), furble_gps.time.hour(),
             furble_gps.time.minute(), furble_gps.time.second());

    if (first || updated) {
      first = false;
      ez.header.draw("gps");
      ez.msgBox("GPS Data", {buffer0, buffer1, buffer2, buffer3}, {"Back"}, false);
    }

    ez.yield();
  } while (true);
}

/**
 * Read GPS enable setting.
 */
bool settings_load_gps() {
  Preferences prefs;

  prefs.begin(FURBLE_STR, true);
  bool enable = prefs.getBool(PREFS_GPS, false);
  prefs.end();

  return enable;
}

/**
 * Save GPS enable setting.
 */
static void settings_save_gps(bool enable) {
  Preferences prefs;

  prefs.begin(FURBLE_STR, false);
  prefs.putBool(PREFS_GPS, enable);
  prefs.end();
}

bool settings_gps_onoff(ezMenu *menu) {
  furble_gps_enable = !furble_gps_enable;
  menu->setCaption("onoff", std::string("GPS\t") + (furble_gps_enable ? "ON" : "OFF"));
  settings_save_gps(furble_gps_enable);

  return true;
}

/**
 * GPS settings menu.
 */
void settings_menu_gps(void) {
  ezMenu submenu(FURBLE_STR " - GPS settings");

  submenu.buttons({"OK", "down"});
  submenu.addItem("onoff", std::string("GPS\t") + (furble_gps_enable ? "ON" : "OFF"), NULL,
                  settings_gps_onoff);
  submenu.addItem("GPS Data", "", show_gps_info);
  submenu.downOnLast("first");
  submenu.addItem("Back");
  submenu.run();
}

void settings_load_interval(interval_t *interval) {
  Preferences prefs;

  prefs.begin(FURBLE_STR, true);
  size_t len = prefs.getBytes(PREFS_INTERVAL, interval, sizeof(interval_t));
  if (len != sizeof(interval_t)) {
    // default values
    interval->count.value = INTERVAL_DEFAULT_COUNT;
    interval->count.unit = INTERVAL_DEFAULT_COUNT_UNIT;
    interval->shutter.value = INTERVAL_DEFAULT_SHUTTER;
    interval->shutter.unit = INTERVAL_DEFAULT_SHUTTER_UNIT;
    interval->delay.value = INTERVAL_DEFAULT_DELAY;
    interval->delay.unit = INTERVAL_DEFAULT_DELAY_UNIT;
  }

  prefs.end();
}

void settings_save_interval(interval_t *interval) {
  Preferences prefs;

  prefs.begin(FURBLE_STR, false);
  prefs.putBytes(PREFS_INTERVAL, interval, sizeof(interval_t));
  prefs.end();
}

static bool configure_count(ezMenu *menu) {
  ezMenu submenu("Count");
  submenu.buttons({"OK", "down"});
  submenu.addItem("Custom");
  submenu.addItem("Infinite");
  submenu.downOnLast("first");

  submenu.runOnce();
  if (submenu.pickName() == "Custom") {
    interval.count.unit = SPIN_UNIT_NIL;
    spinner_modify_value("Count", false, &interval.count);
  }

  if (submenu.pickName() == "Infinite") {
    interval.count.unit = SPIN_UNIT_INF;
  }

  std::string countstr = sv2str(&interval.count);
  if (interval.count.unit == SPIN_UNIT_INF) {
    countstr = "INF";
  }

  menu->setCaption("interval_count", std::string("Count\t") + countstr);
  settings_save_interval(&interval);

  return true;
}

static bool configure_delay(ezMenu *menu) {
  ezMenu submenu("Delay");
  submenu.buttons({"OK", "down"});
  submenu.addItem("Custom");
  submenu.addItem("Preset");
  submenu.downOnLast("first");

  bool preset;

  submenu.runOnce();
  if (submenu.pickName() == "Custom") {
    preset = false;
  }
  if (submenu.pickName() == "Preset") {
    preset = true;
  }

  spinner_modify_value("Delay", preset, &interval.delay);
  menu->setCaption("interval_delay", "Delay\t" + sv2str(&interval.delay));
  settings_save_interval(&interval);

  return true;
}

static bool configure_shutter(ezMenu *menu) {
  ezMenu submenu("Shutter");
  submenu.buttons({"OK", "down"});
  submenu.addItem("Custom");
  submenu.addItem("Preset");
  submenu.downOnLast("first");

  bool preset;

  submenu.runOnce();
  if (submenu.pickName() == "Custom") {
    preset = false;
  }
  if (submenu.pickName() == "Preset") {
    preset = true;
  }

  spinner_modify_value("Shutter", preset, &interval.shutter);
  menu->setCaption("interval_shutter", "Shutter\t" + sv2str(&interval.shutter));
  settings_save_interval(&interval);

  return true;
}

void settings_add_interval_items(ezMenu *submenu) {
  settings_load_interval(&interval);

  submenu->addItem("interval_count", std::string("Count\t") + sv2str(&interval.count), NULL,
                   configure_count);
  submenu->addItem("interval_delay", std::string("Delay\t") + sv2str(&interval.delay), NULL,
                   configure_delay);
  submenu->addItem("interval_shutter", std::string("Shutter\t") + sv2str(&interval.shutter), NULL,
                   configure_shutter);
}

void settings_menu_interval(void) {
  ezMenu submenu(FURBLE_STR " - Intervalometer settings");
  submenu.buttons({"OK", "down"});
  settings_add_interval_items(&submenu);
  submenu.addItem("Back");
  submenu.downOnLast("first");
  submenu.run();
}
