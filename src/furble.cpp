#include <Furble.h>
#include <M5ez.h>
#include <TinyGPS++.h>

#include <M5Unified.h>

#include "furble_ui.h"
#include "interval.h"
#include "settings.h"

const uint32_t SCAN_DURATION = 10;

bool load_gps_enable();

TinyGPSPlus gps;
HardwareSerial GroveSerial(2);
static const uint32_t GPS_BAUD = 9600;
static const uint16_t GPS_SERVICE_MS = 250;
static const uint32_t GPS_MAX_AGE_MS = 60 * 1000;

static const uint8_t CURRENT_POSITION = LEFTMOST + 1;
static const uint8_t GPS_HEADER_POSITION = CURRENT_POSITION + 1;

bool gps_enable = false;
static bool gps_has_fix = false;

/**
 * Progress bar update function.
 */
void update_progress_bar(void *ctx, float value) {
  ezProgressBar *progress_bar = (ezProgressBar *)ctx;
  progress_bar->value(value);
}

/**
 * BLE Advertisement callback.
 */
void onScanResult(std::vector<Furble::Camera *> &list) {
  ez.msgBox("Scanning", "Found ... " + String(list.size()), "", false);
};

/**
 * GPS serial event service handler.
 */
static uint16_t service_grove_gps(void *private_data) {
  if (!gps_enable) {
    return GPS_SERVICE_MS;
  }

  while (Serial2.available() > 0) {
    gps.encode(Serial2.read());
  }

  if ((gps.location.age() < GPS_MAX_AGE_MS) && gps.location.isValid()
      && (gps.date.age() < GPS_MAX_AGE_MS) && gps.date.isValid()
      && (gps.time.age() < GPS_MAX_AGE_MS) && gps.time.age()) {
    gps_has_fix = true;
  } else {
    gps_has_fix = false;
  }

  return GPS_SERVICE_MS;
}

/**
 * Update geotag data.
 */
static void update_geodata(Furble::Camera *camera) {
  if (!gps_enable) {
    return;
  }

  if (gps.location.isUpdated() && gps.location.isValid() && gps.date.isUpdated()
      && gps.date.isValid() && gps.time.isValid() && gps.time.isValid()) {
    Furble::Camera::gps_t dgps = {gps.location.lat(), gps.location.lng(), gps.altitude.meters()};
    Furble::Camera::timesync_t timesync = {gps.date.year(), gps.date.month(),  gps.date.day(),
                                           gps.time.hour(), gps.time.minute(), gps.time.second()};

    camera->updateGeoData(dgps, timesync);
    ez.header.draw("gps");
  }
}

/**
 * Draw GPS enable/fix widget.
 */
static void gps_draw_widget(uint16_t x, uint16_t y) {
  if (!gps_enable) {
    return;
  }

  int16_t r = (ez.theme->header_height * 0.8) / 2;
  int16_t cx = x + r;
  int16_t cy = (ez.theme->header_height / 2);

  if (gps_has_fix) {
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
  Serial.println(ma);
  char s[32] = {0};
  snprintf(s, 32, "%d", ma);
  M5.Lcd.drawString(s, x + ez.theme->header_hmargin, ez.theme->header_tmargin + 2);
}

static uint16_t current_service(void *private_data) {
  ez.header.draw("current");
  return 1000;
}

/**
 * Display the version.
 */
static void about(void) {
  String version = FURBLE_VERSION;
  if (version.length() < 1) {
    version = "unknown";
  }

  ez.msgBox(FURBLE_STR " - About", "Version: " + version, "Back", true);
}

static void show_shutter_control(bool shutter_locked, unsigned long lock_start_ms) {
  static unsigned long prev_update_ms = 0;

  if (shutter_locked) {
    unsigned long now = millis();

    if ((now - prev_update_ms) < 1000) {
      // Don't update if less than 1000ms
      return;
    }

    unsigned long total_ms = now - lock_start_ms;
    unsigned long minutes = total_ms / 1000 / 60;
    unsigned long seconds = (total_ms / 1000) % 60;
    prev_update_ms = now;

    char duration[8] = {0x0};
    snprintf(duration, 8, "%02lu:%02lu", minutes, seconds);

#ifdef M5STACK_CORE2
    ez.msgBox("Remote Shutter", "Shutter Locked|" + String(duration), "Unlock#Unlock#Back", false);
#else
    ez.msgBox("Remote Shutter", "Shutter Locked|" + String(duration) + "||Back: Power",
              "Unlock#Unlock", false);
#endif
  } else {
#ifdef M5STACK_CORE2
    ez.msgBox("Remote Shutter", "Lock: Focus+Release", "Release#Focus#Back", false);
#else
    ez.msgBox("Remote Shutter", "Lock: Focus+Release|Back: Power", "Release#Focus", false);
#endif
  }
}

static void remote_control(FurbleCtx *fctx) {
  Furble::Camera *camera = fctx->camera;
  static unsigned long shutter_lock_start_ms = 0;
  static bool shutter_lock = false;

  Serial.println("Remote Control");

  show_shutter_control(false, 0);

  do {
    M5.update();

    update_geodata(camera);

    if (fctx->reconnected) {
      show_shutter_control(shutter_lock, shutter_lock_start_ms);
      fctx->reconnected = false;
    }

    if (M5.BtnPWR.wasClicked() || M5.BtnC.wasPressed()) {
      if (shutter_lock) {
        // ensure shutter is released on exit
        camera->shutterRelease();
      }
      Serial.println("Exit shutter");
      break;
    }

    if (shutter_lock) {
      // release shutter if either shutter or focus is pressed
      if (M5.BtnA.wasClicked() || M5.BtnB.wasClicked()) {
        shutter_lock = false;
        camera->shutterRelease();
        show_shutter_control(false, 0);
        Serial.println("shutterRelease(unlock)");
      } else {
        show_shutter_control(true, shutter_lock_start_ms);
      }
    } else {
      if (M5.BtnA.wasPressed()) {
        camera->shutterPress();
        Serial.println("shutterPress()");
        continue;
      }

      if (M5.BtnA.wasReleased()) {
        // focus + shutter = shutter lock
        if (M5.BtnB.isPressed()) {
          shutter_lock = true;
          shutter_lock_start_ms = millis();
          show_shutter_control(true, shutter_lock_start_ms);
          Serial.println("shutter lock");
        } else {
          camera->shutterRelease();
          Serial.println("shutterRelease()");
        }
        continue;
      }

      if (M5.BtnB.wasPressed()) {
        camera->focusPress();
        Serial.println("focusPress()");
        continue;
      }

      if (M5.BtnB.wasReleased()) {
        camera->focusRelease();
        Serial.println("focusRelease()");
        continue;
      }
    }

    ez.yield();
    delay(50);
  } while (camera->isConnected());
}

uint16_t disconnectDetect(void *private_data) {
  FurbleCtx *fctx = (FurbleCtx *)private_data;
  Furble::Camera *camera = fctx->camera;

  if (camera->isConnected())
    return 500;

  String buttons = ez.buttons.get();
  String header = ez.header.title();

  ezProgressBar progress_bar(FURBLE_STR, "Reconnecting ...", "");
  if (camera->connect(&update_progress_bar, &progress_bar)) {
    ez.screen.clear();
    ez.header.show(header);
    ez.buttons.show(buttons);

    fctx->reconnected = true;

    ez.redraw();
    return 500;
  }

  ez.screen.clear();
  // no recovery, restart device
  esp_restart();
  return 0;
}

static void menu_remote(FurbleCtx *fctx) {
  ez.backlight.inactivity(NEVER);
  ezMenu submenu(FURBLE_STR " - Connected");
  submenu.buttons("OK#down");
  submenu.addItem("Shutter");
  submenu.addItem("Interval");
  submenu.addItem("Disconnect");
  submenu.downOnLast("first");

  ez.addEvent(disconnectDetect, fctx, 500);

  do {
    submenu.runOnce();

    if (submenu.pickName() == "Shutter") {
      remote_control(fctx);
    }

    if (submenu.pickName() == "Interval") {
      remote_interval(fctx);
    }
  } while (submenu.pickName() != "Disconnect");

  ez.removeEvent(disconnectDetect);

  fctx->camera->disconnect();
  ez.backlight.inactivity(USER_SET);
}

static void menu_connect(bool save) {
  ezMenu submenu(FURBLE_STR " - Connect");
  submenu.buttons("OK#down");

  for (int i = 0; i < Furble::CameraList::m_ConnectList.size(); i++) {
    submenu.addItem(Furble::CameraList::m_ConnectList[i]->getName());
  }
  submenu.addItem("Back");
  submenu.downOnLast("first");
  int16_t i = submenu.runOnce();
  if (i == 0)
    return;

  FurbleCtx fctx = {Furble::CameraList::m_ConnectList[i - 1], false};

  update_geodata(fctx.camera);

  ezProgressBar progress_bar(FURBLE_STR, "Connecting ...", "");
  if (fctx.camera->connect(&update_progress_bar, &progress_bar)) {
    if (save) {
      Furble::CameraList::save(fctx.camera);
    }
    menu_remote(&fctx);
  }
}

/**
 * Scan for devices, then present connection menu.
 */
static void do_scan(void) {
  Furble::CameraList::m_ConnectList.clear();
  Furble::Scan::clear();
  ez.msgBox("Scanning", "Found ... ", "", false);
  Furble::Scan::start(SCAN_DURATION);
  menu_connect(true);
}

/**
 * Retrieve saved devices, then present connection menu.
 */
static void do_saved(void) {
  Furble::CameraList::load();
  menu_connect(false);
}

static void menu_delete(void) {
  ezMenu submenu(FURBLE_STR " - Delete");
  submenu.buttons("OK#down");
  Furble::CameraList::load();

  for (size_t i = 0; i < Furble::CameraList::m_ConnectList.size(); i++) {
    submenu.addItem(Furble::CameraList::m_ConnectList[i]->getName());
  }
  submenu.addItem("Back");
  submenu.downOnLast("first");

  int16_t i = submenu.runOnce();
  if (i == 0)
    return;
  Furble::CameraList::remove(Furble::CameraList::m_ConnectList[i - 1]);
}

static void menu_settings(void) {
  ezMenu submenu(FURBLE_STR " - Settings");

  submenu.buttons("OK#down");
  submenu.addItem("Backlight", ez.backlight.menu);
  submenu.addItem("GPS", settings_menu_gps);
  submenu.addItem("Intervalometer", settings_menu_interval);
  submenu.addItem("Theme", ez.theme->menu);
  submenu.addItem("Transmit Power", settings_menu_tx_power);
  submenu.addItem("About", about);
  submenu.addItem("Back");
  submenu.downOnLast("first");
  submenu.run();
}

static void mainmenu_poweroff(void) {
  M5.Power.powerOff();
}

void setup() {
  gps_enable = load_gps_enable();

  Serial.begin(115200);
  Serial2.begin(GPS_BAUD, SERIAL_8N1, 33, 32);

#include <themes/dark.h>
#include <themes/default.h>
#include <themes/mono_furble.h>

  ez.begin();

  uint8_t width = 4 * M5.Lcd.textWidth("5") + ez.theme->header_hmargin * 2;
  ez.header.insert(CURRENT_POSITION, "current", width, current_draw_widget);
  ez.header.insert(GPS_HEADER_POSITION, "gps", ez.theme->header_height * 0.8, gps_draw_widget);
  ez.addEvent(service_grove_gps, nullptr, millis() + 500);
  ez.addEvent(current_service, nullptr, millis() + 500);

  Furble::Scan::init(onScanResult);

  // Set BLE transmit power
  esp_power_level_t esp_power = settings_load_esp_tx_power();
  Furble::setPower(esp_power);
}

void loop() {
  ezMenu mainmenu(FURBLE_STR);
  mainmenu.buttons("OK#down");
  mainmenu.addItem("Connect", do_saved);
  mainmenu.addItem("Scan", do_scan);
  mainmenu.addItem("Delete Saved", menu_delete);
  mainmenu.addItem("Settings", menu_settings);
  mainmenu.addItem("Power Off", mainmenu_poweroff);
  mainmenu.downOnLast("first");
  mainmenu.run();
}
