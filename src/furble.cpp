#include <Device.h>
#include <Furble.h>
#include <M5ez.h>

#include <M5Unified.h>

#include "furble_control.h"
#include "furble_gps.h"
#include "furble_ui.h"
#include "interval.h"
#include "settings.h"

const uint32_t SCAN_DURATION = (60 * 5);
static Furble::Control *g_Control;

/**
 * Progress bar update function.
 */
void update_progress_bar(void *ctx, float value) {
  ezProgressBar *progress_bar = static_cast<ezProgressBar *>(ctx);
  progress_bar->value(value);
}

/**
 * Display the version.
 */
static void about(void) {
  std::string version = FURBLE_VERSION;
  if (version.length() < 1) {
    version = "unknown";
  }

  ez.msgBox(
      FURBLE_STR " - About",
      {std::string("Version: ") + version, std::string("ID: ") + Furble::Device::getStringID()},
      {"Back"}, true);
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

    char duration[16] = {0x0};
    snprintf(duration, 16, "%02lu:%02lu", minutes, seconds);

#if ARDUINO_M5STACK_CORE_ESP32 || ARDUINO_M5STACK_CORE2
    ez.msgBox("Remote Shutter", {std::string("Shutter Locked") + std::string(duration)},
              {"Unlock", "Unlock", "Back"}, false);
#else
    ez.msgBox("Remote Shutter",
              {std::string("Shutter Locked") + std::string(duration), "", "Back: Power"},
              {"Unlock", "Unlock"}, false);
#endif
  } else {
#if ARDUINO_M5STACK_CORE_ESP32 || ARDUINO_M5STACK_CORE2
    ez.msgBox("Remote Shutter", {"Lock: Focus+Release"}, {"Release", "Focus", "Back"}, false);
#else
    ez.msgBox("Remote Shutter", {"Lock: Focus+Release", "Back: Power"}, {"Release", "Focus"},
              false);
#endif
  }
}

static void remote_control(FurbleCtx *fctx) {
  auto control = fctx->control;
  static unsigned long shutter_lock_start_ms = 0;
  static bool shutter_lock = false;

  ESP_LOGI(LOG_TAG, "Remote Control");

  show_shutter_control(false, 0);

  do {
    ez.yield();

    if (fctx->reconnected) {
      show_shutter_control(shutter_lock, shutter_lock_start_ms);
      fctx->reconnected = false;
    }

    if (M5.BtnPWR.wasClicked() || M5.BtnC.wasPressed()) {
      if (shutter_lock) {
        // ensure shutter is released on exit
        control->sendCommand(CONTROL_CMD_SHUTTER_RELEASE);
      }
      ESP_LOGI(LOG_TAG, "Exit shutter");
      break;
    }

    if (shutter_lock) {
      // release shutter if either shutter or focus is pressed
      if (M5.BtnA.wasClicked() || M5.BtnB.wasClicked()) {
        shutter_lock = false;
        control->sendCommand(CONTROL_CMD_SHUTTER_RELEASE);
        show_shutter_control(false, 0);
      } else {
        show_shutter_control(true, shutter_lock_start_ms);
      }
    } else {
      if (M5.BtnA.wasPressed()) {
        control->sendCommand(CONTROL_CMD_SHUTTER_PRESS);
        continue;
      }

      if (M5.BtnA.wasReleased()) {
        // focus + shutter = shutter lock
        if (M5.BtnB.isPressed()) {
          shutter_lock = true;
          shutter_lock_start_ms = millis();
          show_shutter_control(true, shutter_lock_start_ms);
          ESP_LOGI(LOG_TAG, "shutter lock");
        } else {
          control->sendCommand(CONTROL_CMD_SHUTTER_RELEASE);
        }
        continue;
      }

      if (M5.BtnB.wasPressed()) {
        control->sendCommand(CONTROL_CMD_FOCUS_PRESS);
        continue;
      }

      if (M5.BtnB.wasReleased()) {
        control->sendCommand(CONTROL_CMD_FOCUS_RELEASE);
        continue;
      }
    }
  } while (control->isConnected());
}

/**
 * Refresh the camera connection status, this includes:
 * * disconnect detection
 * * geotag updates
 */
static uint16_t statusRefresh(void *private_data) {
  FurbleCtx *fctx = static_cast<FurbleCtx *>(private_data);
  auto control = fctx->control;

  if (control->isConnected()) {
    furble_gps_update(control);
    return 500;
  }

  auto buttons = ez.buttons.get();
  std::string header = ez.header.title();

  for (const auto &target : control->getTargets()) {
    auto camera = target->getCamera();
    if (!camera->isConnected()) {
      ezProgressBar progress_bar(FURBLE_STR, {"Reconnecting ..."}, {""});
      if (camera->connect(settings_load_esp_tx_power(), &update_progress_bar, &progress_bar)) {
        ez.screen.clear();
        ez.header.show(header);
        ez.buttons.show(buttons);

        fctx->reconnected = true;

        ez.redraw();
        return 500;
      }
    }
  }

  ez.screen.clear();
  // no recovery, restart device
  esp_restart();
  return 0;
}

static void menu_remote(FurbleCtx *fctx) {
  ez.backlight.inactivity(NEVER);
  ezMenu submenu(FURBLE_STR " - Connected");
  submenu.buttons({"OK", "down"});
  submenu.addItem("Shutter");
  submenu.addItem("Interval");
  submenu.addItem("Disconnect");
  submenu.downOnLast("first");

  ez.addEvent(statusRefresh, fctx, 500);

  do {
    submenu.runOnce();

    if (submenu.pickName() == "Shutter") {
      remote_control(fctx);
    }

    if (submenu.pickName() == "Interval") {
      remote_interval(fctx);
    }
  } while (submenu.pickName() != "Disconnect");

  ez.removeEvent(statusRefresh);

  fctx->control->disconnect();
  ez.backlight.inactivity(USER_SET);
}

/**
 * Scan callback to update connection menu with new devices.
 */
void updateConnectItems(void *private_data) {
  ezMenu *submenu = (ezMenu *)private_data;

  submenu->deleteItem("Back");
  for (int i = submenu->countItems(); i < Furble::CameraList::size(); i++) {
    submenu->addItem(Furble::CameraList::get(i)->getName());
  }
  submenu->addItem("Back");
}

static void menu_connect(Furble::Control *control, bool scan) {
  std::string header = FURBLE_STR " - ";
  if (scan) {
    header += "Scanning";
  } else {
    header += "Connect";
  }

  ezMenu submenu(header);
  if (scan) {
    ez.backlight.inactivity(NEVER);
    Furble::Scan::start(SCAN_DURATION, updateConnectItems, &submenu);
  } else {
    updateConnectItems(&submenu);
  }

  submenu.buttons({"OK", "down"});
  if (submenu.getItemNum("Back") == 0) {
    submenu.addItem("Back");
  }
  submenu.downOnLast("first");

  submenu.runOnce(true);

  if (scan) {
    Furble::Scan::stop();
    ez.backlight.inactivity(USER_SET);
  }

  int16_t i = submenu.pick();
  if (i == 0)
    return;

  // FurbleCtx fctx = {Furble::CameraList::get(i - 1), false};
  FurbleCtx fctx = {control, false};
  auto camera = Furble::CameraList::get(i - 1);

  ezProgressBar progress_bar(FURBLE_STR, {std::string("Connecting to ") + camera->getName()}, {""});
  if (camera->connect(settings_load_esp_tx_power(), &update_progress_bar, &progress_bar)) {
    if (scan) {
      Furble::CameraList::save(camera);
    }
    control->addActive(camera);
    menu_remote(&fctx);
  }
}

/**
 * Scan for devices, then present connection menu.
 */
static void do_scan(void) {
  Furble::CameraList::clear();
  Furble::Scan::clear();
  menu_connect(g_Control, true);
}

/**
 * Retrieve saved devices, then present connection menu.
 */
static void do_saved(void) {
  Furble::CameraList::load();
  menu_connect(g_Control, false);
}

static void menu_delete(void) {
  ezMenu submenu(FURBLE_STR " - Delete");
  submenu.buttons({"OK", "down"});
  Furble::CameraList::load();

  for (size_t i = 0; i < Furble::CameraList::size(); i++) {
    submenu.addItem(Furble::CameraList::get(i)->getName());
  }
  submenu.addItem("Back");
  submenu.downOnLast("first");

  int16_t i = submenu.runOnce();
  if (i == 0)
    return;
  Furble::CameraList::remove(Furble::CameraList::get(i - 1));
}

static void menu_settings(void) {
  ezMenu submenu(FURBLE_STR " - Settings");

  submenu.buttons({"OK", "down"});
  submenu.addItem("Backlight", "", ez.backlight.menu);
  submenu.addItem("GPS", "", settings_menu_gps);
  submenu.addItem("Intervalometer", "", settings_menu_interval);
  submenu.addItem("Theme", "", ez.theme->menu);
  submenu.addItem("Transmit Power", "", settings_menu_tx_power);
  submenu.addItem("About", "", about);
  submenu.addItem("Back");
  submenu.downOnLast("first");
  submenu.run();
}

static void mainmenu_poweroff(void) {
  M5.Power.powerOff();
}

void vUITask(void *param) {
  g_Control = static_cast<Furble::Control *>(param);

#include <themes/dark.h>
#include <themes/default.h>
#include <themes/mono_furble.h>

  ez.begin();
  furble_gps_init();

  while (true) {
    size_t save_count = Furble::CameraList::getSaveCount();

    ezMenu mainmenu(FURBLE_STR);
    mainmenu.buttons({"OK", "down"});
    if (save_count > 0) {
      mainmenu.addItem("Connect", "", do_saved);
    }
    mainmenu.addItem("Scan", "", do_scan);
    if (save_count > 0) {
      mainmenu.addItem("Delete Saved", "", menu_delete);
    }
    mainmenu.addItem("Settings", "", menu_settings);
    mainmenu.addItem("Power Off", "", mainmenu_poweroff);
    mainmenu.downOnLast("first");

    do {
      mainmenu.runOnce();
    } while (Furble::CameraList::getSaveCount() == save_count);
  }
}
