#include <M5Unified.h>

#include <Device.h>
#include <Furble.h>
#include <M5ez.h>

#include "furble_control.h"
#include "furble_gps.h"
#include "furble_ui.h"
#include "interval.h"
#include "settings.h"

typedef struct {
  Furble::Control *control;
  ezMenu *menu;
  bool scan;
  bool multiconnect;
} ui_context_t;

#define CONNECT_STAR "Connect *"

/**
 * Get seconds since boot.
 */
static uint64_t get_time_secs(void) {
  return (esp_timer_get_time() / 1000000LL);
}

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
    ez.msgBox("Remote Shutter", {std::string("Shutter Locked"), std::string(duration)},
              {"Unlock", "Unlock", "Back"}, false);
#else
    ez.msgBox("Remote Shutter",
              {std::string("Shutter Locked"), std::string(duration), "", "Back: Power"},
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
  auto *control = fctx->control;
  static unsigned long shutter_lock_start_ms = 0;
  static bool shutter_lock = false;

  ESP_LOGI(LOG_TAG, "Remote Control");

  show_shutter_control(false, 0);

  do {
    ez.yield();

    // show_shutter_control(shutter_lock, shutter_lock_start_ms);

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
  } while (!fctx->cancelled && control->allConnected());
}

/**
 * Attempt a connection with a progress bar.
 *
 * @return false if the attempt was cancelled
 */
static bool connect_with_progress(Furble::Control *control, Furble::Camera *camera) {
  ezProgressBar progress_bar(FURBLE_STR, {std::string("Connecting to"), camera->getName()},
                             {"", "Cancel"});
  control->connect(camera, settings_load_esp_tx_power());

  // Wait up to 30 seconds
  uint64_t timeout = get_time_secs() + 30;
  while (get_time_secs() <= timeout) {
    progress_bar.value(camera->getConnectProgress());
    ez.yield(false);
    if (M5.BtnB.wasClicked()) {
      // issue a cancel to the backend stack
      ble_gap_conn_cancel();
      return false;
    }
    if (camera->isConnected()) {
      return true;
    }
  }

  return true;
}

/**
 * Refresh the camera connection status, this includes:
 * * disconnect detection
 * * geotag updates
 */
static uint16_t statusRefresh(void *context) {
  FurbleCtx *fctx = static_cast<FurbleCtx *>(context);
  auto *control = fctx->control;

  if (control->allConnected()) {
    furble_gps_update(control);
    return 500;
  }

  if (fctx->cancelled) {
    return 500;
  }

  auto buttons = ez.buttons.get();
  std::string header = ez.header.title();

  for (const auto &target : control->getTargets()) {
    auto camera = target->getCamera();
    if (!camera->isConnected()) {
      if (!connect_with_progress(control, camera)) {
        fctx->cancelled = true;
      }
      ez.screen.clear();
      ez.header.show(header);
      ez.buttons.show(buttons);
      ez.redraw();

      return 500;
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
    if (fctx->cancelled) {
      break;
    }

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

static bool do_connect(ezMenu *menu, void *context) {
  auto *ctx = static_cast<ui_context_t *>(context);

  FurbleCtx fctx = {ctx->control, false};

  if (!ctx->scan && ctx->multiconnect) {
    for (int n = 0; n < Furble::CameraList::size(); n++) {
      auto *camera = Furble::CameraList::get(n);
      if (camera->isActive()) {
        connect_with_progress(ctx->control, camera);
        if (camera->isConnected()) {
          ctx->control->addActive(camera);
        } else {
          // Fail all if any connect fails
          return false;
        }
      }
    }
  } else {
    auto *camera = Furble::CameraList::get(menu->pick() - 1);
    connect_with_progress(ctx->control, camera);

    if (camera->isConnected()) {
      if (ctx->scan) {
        Furble::CameraList::save(camera);
      }
      ctx->control->addActive(camera);
    } else {
      camera->disconnect();
      return false;
    }
  }

  menu_remote(&fctx);
  return true;
}

static bool toggleCameraSelect(ezMenu *menu, void *context) {
  auto *camera = static_cast<Furble::Camera *>(context);
  camera->setActive(!camera->isActive());
  menu->setCaption(camera->getAddress().toString(),
                   camera->getName() + "\t" + (camera->isActive() ? "*" : ""));

  return true;
}

/**
 * Scan callback to update connection menu with new devices.
 */
static void updateConnectItems(void *context) {
  auto *ctx = static_cast<ui_context_t *>(context);
  auto *menu = ctx->menu;

  if (!ctx->scan && ctx->multiconnect) {
    menu->deleteItem(CONNECT_STAR);
  }
  menu->deleteItem("Back");
  for (int i = menu->countItems(); i < Furble::CameraList::size(); i++) {
    auto *camera = Furble::CameraList::get(i);

    if (!ctx->scan && ctx->multiconnect) {
      menu->addItem(camera->getAddress().toString(),
                    camera->getName() + "\t" + (camera->isActive() ? "*" : ""), NULL,
                    static_cast<void *>(camera), toggleCameraSelect);
    } else {
      menu->addItem(camera->getAddress().toString(), camera->getName(), NULL, ctx, do_connect);
    }
  }
  if (!ctx->scan && ctx->multiconnect) {
    menu->addItem(CONNECT_STAR, "", NULL, ctx, do_connect);
  }
  menu->addItem("Back");
}

static void menu_connect(Furble::Control *control, bool scan) {
  std::string header = FURBLE_STR " - ";
  if (scan) {
    header += "Scanning";
  } else {
    header += "Connect";
  }

  ezMenu submenu(header);
  ui_context_t ui_ctx = (ui_context_t){control, &submenu, scan, settings_load_multiconnect()};
  if (scan) {
    ez.backlight.inactivity(NEVER);
    Furble::Scan::start(updateConnectItems, &ui_ctx);
  } else {
    updateConnectItems(&ui_ctx);
  }

  submenu.buttons({"OK", "down"});
  if (submenu.getItemNum("Back") == 0) {
    submenu.addItem("Back");
  }
  submenu.downOnLast("first");

  do {
    submenu.runOnce(true);

    if (scan) {
      Furble::Scan::stop();
      ez.backlight.inactivity(USER_SET);
    }

    if (submenu.pick() == 0) {
      break;
    }
  } while (!scan);
}

/**
 * Scan for devices, then present connection menu.
 */
static bool do_scan(ezMenu *menu, void *context) {
  auto *control = static_cast<Furble::Control *>(context);

  Furble::CameraList::clear();
  Furble::Scan::clear();
  menu_connect(control, true);

  return true;
}

/**
 * Retrieve saved devices, then present connection menu.
 */
static bool do_saved(ezMenu *menu, void *context) {
  auto *control = static_cast<Furble::Control *>(context);

  Furble::CameraList::load();
  menu_connect(control, false);

  return true;
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

/**
 * Toggle Multi-Connect menu setting.
 */
static bool multiconnect_toggle(ezMenu *menu, void *context) {
  bool *multiconnect = static_cast<bool *>(context);
  *multiconnect = !*multiconnect;
  menu->setCaption("multiconnectonoff",
                   std::string("Multi-Connect\t") + (*multiconnect ? "ON" : "OFF"));

  settings_save_multiconnect(*multiconnect);

  return true;
}

static void menu_settings(void) {
  ezMenu submenu(FURBLE_STR " - Settings");

  bool multiconnect = settings_load_multiconnect();

  submenu.buttons({"OK", "down"});
  submenu.addItem("Backlight", "", ez.backlight.menu);
  submenu.addItem("GPS", "", settings_menu_gps);
  submenu.addItem("Intervalometer", "", settings_menu_interval);
  submenu.addItem("multiconnectonoff",
                  std::string("Multi-Connect\t") + (multiconnect ? "ON" : "OFF"), nullptr,
                  &multiconnect, multiconnect_toggle);
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
  auto *control = static_cast<Furble::Control *>(param);

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
      mainmenu.addItem("Connect", "", nullptr, control, do_saved);
    }
    mainmenu.addItem("Scan", "", nullptr, control, do_scan);
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
