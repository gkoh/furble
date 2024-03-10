#include <Furble.h>
#include <M5ez.h>

#include "furble_ui.h"
#include "interval.h"
#include "settings.h"
#include "spinner.h"

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
  static char prev_hms[32] = {0x0};
  char hms[32] = {0x0};
  int len = 0;

  if (sv_count->unit == SPIN_UNIT_INF) {
    len = snprintf(hms, 32, "%09u|%02u:%02u:%02u", count, rem_h, rem_m, rem_s);
  } else {
    len =
        snprintf(hms, 32, "%03u/%03u|%02u:%02u:%02u", count, sv_count->value, rem_h, rem_m, rem_s);
  }
  // Serial.println(hms);

  if ((len > 0) && memcmp(prev_hms, hms, len)) {
    memcpy(prev_hms, hms, len);
    ez.msgBox((String)statestr, (String)hms, "Stop", false, NULL, NO_COLOR, false);
  }
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
  ezMenu submenu(FURBLE_STR " - Interval");
  submenu.buttons("OK#down");
  submenu.addItem("Start");
  settings_add_interval_items(&submenu);
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
