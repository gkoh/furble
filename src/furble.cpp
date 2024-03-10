#include <Furble.h>
#include <M5ez.h>

#include <M5Unified.h>

#include "furble_gps.h"
#include "furble_ui.h"
#include "interval.h"
#include "settings.h"

const uint32_t SCAN_DURATION = 10;

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

    furble_gps_update_geodata(camera);

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
  if (camera->connect(settings_load_esp_tx_power(), &update_progress_bar, &progress_bar)) {
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

  furble_gps_update_geodata(fctx.camera);

  ezProgressBar progress_bar(FURBLE_STR, "Connecting ...", "");
  if (fctx.camera->connect(settings_load_esp_tx_power(), &update_progress_bar, &progress_bar)) {
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
  Serial.begin(115200);

#include <themes/dark.h>
#include <themes/default.h>
#include <themes/mono_furble.h>

  ez.begin();
  furble_gps_init();

  Furble::Scan::init(settings_load_esp_tx_power(), onScanResult);
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
