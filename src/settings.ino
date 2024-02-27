const char *PREFS_TX_POWER = "txpower";
const char *PREFS_GPS = "gps";

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
  ezProgressBar power_bar(FURBLE_STR, "Set transmit power", "Adjust#Back");
  power_bar.value(power / 0.03f);
  while (true) {
    String b = ez.buttons.poll();
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
  char buffer[256] = {0x0};
  bool first = true;

  do {
    bool updated = gps.location.isUpdated() || gps.date.isUpdated() || gps.time.isUpdated();

    snprintf(
        buffer, 256, "%s (%d) | %.2f, %.2f | %.2f metres | %4u-%02u-%02u %02u:%02u:%02u",
        gps.location.isValid() && gps.date.isValid() && gps.time.isValid() ? "Valid" : "Invalid",
        gps.location.age(), gps.location.lat(), gps.location.lng(), gps.altitude.meters(),
        gps.date.year(), gps.date.month(), gps.date.day(), gps.time.hour(), gps.time.minute(),
        gps.time.second());

    if (first || updated) {
      first = false;
      ez.header.draw("gps");
      ez.msgBox("GPS Data", buffer, "Back", false);
    }

    M5.update();

    if (M5.BtnB.wasPressed()) {
      break;
    }

    ez.yield();
    delay(100);
  } while (true);
}

/**
 * Read GPS enable setting.
 */
bool load_gps_enable() {
  Preferences prefs;

  prefs.begin(FURBLE_STR, true);
  bool enable = prefs.getBool(PREFS_GPS, false);
  prefs.end();

  return enable;
}

/**
 * Save GPS enable setting.
 */
static void save_gps_enable(bool enable) {
  Preferences prefs;

  prefs.begin(FURBLE_STR, false);
  prefs.putBool(PREFS_GPS, enable);
  prefs.end();
}

bool gps_onoff(ezMenu *menu) {
  gps_enable = !gps_enable;
  menu->setCaption("onoff", "GPS\t" + (String)(gps_enable ? "ON" : "OFF"));
  save_gps_enable(gps_enable);

  return true;
}

/**
 * GPS settings menu.
 */
void settings_menu_gps(void) {
  ezMenu submenu(FURBLE_STR " - GPS settings");

  submenu.buttons("OK#down");
  submenu.addItem("onoff | GPS\t" + (String)(gps_enable ? "ON" : "OFF"), NULL, gps_onoff);
  submenu.addItem("GPS Data", show_gps_info);
  submenu.downOnLast("first");
  submenu.addItem("Back");
  submenu.run();
}
