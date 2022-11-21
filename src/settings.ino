const char *PREFS_TX_POWER = "txpower";

static Preferences prefs;

/**
 * Save BLE transmit power to preferences.
 */
static void save_tx_power(uint8_t tx_power) {
  prefs.begin(FURBLE_STR, false);
  prefs.putUChar(PREFS_TX_POWER, tx_power);
  prefs.end();
}

/**
 * Load BLE transmit power from preferences.
 */
static uint8_t load_tx_power() {
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
