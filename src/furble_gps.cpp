#include <Camera.h>
#include <M5Unified.h>
#include <M5ez.h>
#include <TinyGPS++.h>

#include "furble_control.h"
#include "furble_gps.h"
#include "settings.h"

static const uint32_t GPS_BAUD = 9600;
#if ARDUINO_M5STACK_CORE_ESP32 || ARDUINO_M5STACK_CORE2
static const int8_t GPS_RX = 22;
static const int8_t GPS_TX = 21;
#else
static const int8_t GPS_RX = 33;
static const int8_t GPS_TX = 32;
#endif
static const uint16_t GPS_SERVICE_MS = 250;
static const uint32_t GPS_MAX_AGE_MS = 60 * 1000;

static const uint8_t CURRENT_POSITION = LEFTMOST + 1;
static const uint8_t GPS_HEADER_POSITION = CURRENT_POSITION + 1;

static bool furble_gps_has_fix = false;

TinyGPSPlus furble_gps;
bool furble_gps_enable = false;

/**
 * GPS serial event service handler.
 */
static uint16_t service_grove_gps(void *context) {
  if (!furble_gps_enable) {
    return GPS_SERVICE_MS;
  }

  while (Serial2.available() > 0) {
    furble_gps.encode(Serial2.read());
  }

  if ((furble_gps.location.age() < GPS_MAX_AGE_MS) && furble_gps.location.isValid()
      && (furble_gps.date.age() < GPS_MAX_AGE_MS) && furble_gps.date.isValid()
      && (furble_gps.time.age() < GPS_MAX_AGE_MS) && furble_gps.time.age()) {
    furble_gps_has_fix = true;
  } else {
    furble_gps_has_fix = false;
  }

  return GPS_SERVICE_MS;
}

/**
 * Update geotag data.
 */
void furble_gps_update(Furble::Control *control) {
  if (!furble_gps_enable) {
    return;
  }

  if (furble_gps.location.isUpdated() && furble_gps.location.isValid()
      && furble_gps.date.isUpdated() && furble_gps.date.isValid() && furble_gps.time.isValid()
      && furble_gps.time.isValid()) {
    Furble::Camera::gps_t dgps = {furble_gps.location.lat(), furble_gps.location.lng(),
                                  furble_gps.altitude.meters()};
    Furble::Camera::timesync_t timesync = {furble_gps.date.year(),   furble_gps.date.month(),
                                           furble_gps.date.day(),    furble_gps.time.hour(),
                                           furble_gps.time.minute(), furble_gps.time.second()};

    control->updateGPS(dgps, timesync);
    ez.header.draw("gps");
  }
}

/**
 * Draw GPS enable/fix widget.
 */
static void gps_draw_widget(uint16_t x, uint16_t y) {
  if (!furble_gps_enable) {
    return;
  }

  int16_t r = (ez.theme->header_height * 0.8) / 2;
  int16_t cx = x + r;
  int16_t cy = (ez.theme->header_height / 2);

  if (furble_gps_has_fix) {
    // With fix, draw solid circle
    M5.Lcd.fillCircle(cx, cy, r, ez.theme->header_fgcolor);
  } else {
    // No fix, empty circle
    M5.Lcd.drawCircle(cx, cy, r, ez.theme->header_fgcolor);
  }
}

static void current_draw_widget(uint16_t x, uint16_t y) {
  // hard disable for now
  return;

  M5.Lcd.fillRect(x, 0, y, ez.theme->header_height, ez.theme->header_bgcolor);
  M5.Lcd.setTextColor(ez.theme->header_fgcolor);
  M5.Lcd.setTextDatum(TL_DATUM);
  int32_t ma = M5.Power.getBatteryCurrent();
  ESP_LOGI(LOG_TAG, "%d", ma);
  char s[32] = {0};
  snprintf(s, 32, "%d", ma);
  M5.Lcd.drawString(s, x + ez.theme->header_hmargin, ez.theme->header_tmargin + 2);
}

static uint16_t current_service(void *context) {
  ez.header.draw("current");
  return 1000;
}

void furble_gps_init(void) {
  furble_gps_enable = settings_load_gps();
  Serial2.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);

  uint8_t width = 4 * M5.Lcd.textWidth("5") + ez.theme->header_hmargin * 2;
  ez.header.insert(CURRENT_POSITION, "current", width, current_draw_widget);
  ez.header.insert(GPS_HEADER_POSITION, "gps", ez.theme->header_height * 0.8, gps_draw_widget);
  ez.addEvent(service_grove_gps, nullptr, millis() + 500);
  ez.addEvent(current_service, nullptr, millis() + 500);
}
