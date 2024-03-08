#include <Furble.h>
#include <M5ez.h>

#include "furble_ui.h"
#include "interval.h"
#include "settings.h"
#include "spinner.h"

static interval_t interval;

static bool configure_count(ezMenu *menu) {
  ezMenu submenu("Count");
  submenu.buttons("OK#down");
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

  String countstr = sv2str(&interval.count);
  if (interval.count.unit == SPIN_UNIT_INF) {
    countstr = "INF";
  }

  menu->setCaption("interval_count", "Count\t" + countstr);
  settings_save_interval(&interval);

  return true;
}

static bool configure_delay(ezMenu *menu) {
  ezMenu submenu("Delay");
  submenu.buttons("OK#down");
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
  submenu.buttons("OK#down");
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

enum interval_state_t {
  INTERVAL_SHUTTER_OPEN = 0,
  INTERVAL_SHUTTER_WAIT = 1,
  INTERVAL_SHUTTER_CLOSE = 2,
  INTERVAL_DELAY = 3,
  INTERVAL_EXIT = 4
};

static void display_interval_msg(interval_state_t state,
                                 unsigned int count,
                                 SpinValue *sv_count,
                                 unsigned long now,
                                 unsigned long next) {
  static unsigned long prev_update_ms = 0;

  if ((now - prev_update_ms) < 500) {
    // Don't update if less than 500ms
    return;
  }

  unsigned int rem_h = 0;
  unsigned int rem_m = 0;
  unsigned int rem_s = 0;

  const char *statestr = NULL;
  switch (state) {
    case INTERVAL_SHUTTER_OPEN:
    case INTERVAL_SHUTTER_WAIT:
      statestr = "SHUTTER OPEN";
      break;
    case INTERVAL_SHUTTER_CLOSE:
    case INTERVAL_DELAY:
      statestr = "DELAY";
      break;
  }

  unsigned long rem = next - now;
  if (now > next) {
    rem = 0;
  }

  ms2hms(rem, &rem_h, &rem_m, &rem_s);
  char hms[32] = {0x0};
  if (sv_count->unit == SPIN_UNIT_INF) {
    snprintf(hms, 32, "%09u|%02u:%02u:%02u", count, rem_h, rem_m, rem_s);
  } else {
    snprintf(hms, 32, "%03u/%03u|%02u:%02u:%02u", count, sv_count->value, rem_h, rem_m, rem_s);
  }
  // Serial.println(hms);

  prev_update_ms = now;
  ez.msgBox((String)statestr, (String)hms, "Stop", false);
}

static void do_interval(FurbleCtx *fctx, interval_t *interval) {
  Furble::Device *device = fctx->device;
  const unsigned long config_delay = sv2ms(&interval->delay);
  const unsigned long config_shutter = sv2ms(&interval->shutter);

  unsigned int icount = 0;
  unsigned long idelay = 0;
  unsigned long ishutter = 0;

  interval_state_t state = INTERVAL_SHUTTER_OPEN;

  bool active = true;

  unsigned long now = 0;
  unsigned long next = 0;

  ez.backlight.inactivity(NEVER);

  do {
    now = millis();
    M5.update();

    if (fctx->reconnected) {
      fctx->reconnected = false;
    }

    switch (state) {
      case INTERVAL_SHUTTER_OPEN:
        if ((icount < interval->count.value) || (interval->count.unit == SPIN_UNIT_INF)) {
          // Serial.println("Shutter Open");
          device->shutterPress();
          next = now + config_shutter;
          state = INTERVAL_SHUTTER_WAIT;
        } else {
          state = INTERVAL_EXIT;
        }
        break;
      case INTERVAL_SHUTTER_WAIT:
        if (now > next) {
          state = INTERVAL_SHUTTER_CLOSE;
        }
        break;
      case INTERVAL_SHUTTER_CLOSE:
        icount++;
        // Serial.println("Shutter Release");
        device->shutterRelease();
        next = now + config_delay;
        if ((icount < interval->count.value) || (interval->count.unit == SPIN_UNIT_INF)) {
          state = INTERVAL_DELAY;
        } else {
          state = INTERVAL_EXIT;
        }
        break;
      case INTERVAL_DELAY:
        if (now > next) {
          state = INTERVAL_SHUTTER_OPEN;
        }
        break;
      case INTERVAL_EXIT:
        active = false;
        break;
    }

    if (M5.BtnB.wasClicked()) {
      if (state == INTERVAL_SHUTTER_WAIT) {
        // Serial.print("Shutter Release");
        device->shutterRelease();
      }
      active = false;
    }

    ez.yield();
    display_interval_msg(state, icount, &interval->count, now, next);
    delay(10);
  } while (active && device->isConnected());
  ez.backlight.inactivity(USER_SET);
}

void remote_interval(FurbleCtx *fctx) {
  settings_load_interval(&interval);

  ezMenu submenu(FURBLE_STR " - Interval");
  submenu.buttons("OK#down");
  submenu.addItem("Start");
  submenu.addItem("interval_count | Count\t" + sv2str(&interval.count), NULL, configure_count);
  submenu.addItem("interval_delay | Delay\t" + sv2str(&interval.delay), NULL, configure_delay);
  submenu.addItem("interval_shutter | Shutter\t" + sv2str(&interval.shutter), NULL,
                  configure_shutter);
  submenu.addItem("Back");
  submenu.downOnLast("first");

  do {
    int16_t i = submenu.runOnce();

    if (submenu.pickName() == "Start") {
      do_interval(fctx, &interval);
    }

    if (i == 0) {
      return;
    }

  } while (submenu.pickName() != "Back");
}