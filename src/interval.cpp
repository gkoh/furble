#include <Furble.h>
#include <M5ez.h>

#include "furble_gps.h"
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
    case INTERVAL_EXIT:
      break;
  }

  unsigned long rem = next - now;
  if (now > next) {
    rem = 0;
  }

  ms2hms(rem, &rem_h, &rem_m, &rem_s);
  static char prev_count[16] = {0x0};
  static char prev_hms[32] = {0x0};
  char ccount[16] = {0x0};
  char hms[32] = {0x0};
  int len = 0;
  int clen = 0;

  if (sv_count->unit == SPIN_UNIT_INF) {
    clen = snprintf(ccount, 16, "%09u", count);
    len = snprintf(hms, 32, "%02u:%02u:%02u", rem_h, rem_m, rem_s);
  } else {
    clen = snprintf(ccount, 16, "%03u/%03u", count, sv_count->value);
    len = snprintf(hms, 32, "%02u:%02u:%02u", rem_h, rem_m, rem_s);
  }
  // ESP_LOGI(LOG_TAG, hms);

  if (((len > 0) && memcmp(prev_hms, hms, len))
      || ((clen > 0) && memcmp(prev_count, ccount, clen))) {
    memcpy(prev_hms, hms, len);
    memcpy(prev_count, ccount, clen);
    ez.msgBox(statestr, {ccount, hms}, {"Stop"}, false, NULL, NO_COLOR, false);
  }
}

static void do_interval(FurbleCtx *fctx, interval_t *interval) {
  auto control = fctx->control;
  const unsigned long config_delay = sv2ms(&interval->delay);
  const unsigned long config_shutter = sv2ms(&interval->shutter);

  unsigned int icount = 0;

  interval_state_t state = INTERVAL_SHUTTER_OPEN;

  bool active = true;

  unsigned long now = 0;
  unsigned long next = 0;

  ez.backlight.inactivity(NEVER);

  do {
    ez.yield();
    now = millis();

    if (fctx->reconnected) {
      fctx->reconnected = false;
    }

    switch (state) {
      case INTERVAL_SHUTTER_OPEN:
        if ((icount < interval->count.value) || (interval->count.unit == SPIN_UNIT_INF)) {
          // ESP_LOGI(LOG_TAG, "Shutter Open");
          control->sendCommand(CONTROL_CMD_SHUTTER_PRESS);
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
        // ESP_LOGI(LOG_TAG, "Shutter Release");
        control->sendCommand(CONTROL_CMD_SHUTTER_RELEASE);
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
        // ESP_LOGI(LOG_TAG, "Shutter Release");
        control->sendCommand(CONTROL_CMD_SHUTTER_RELEASE);
      }
      active = false;
    }

    display_interval_msg(state, icount, &interval->count, now, next);
  } while (active && control->isConnected());
  ez.backlight.inactivity(USER_SET);
}

void remote_interval(FurbleCtx *fctx) {
  ezMenu submenu(FURBLE_STR " - Interval");
  submenu.buttons({"OK", "down"});
  submenu.addItem("Start");
  settings_add_interval_items(&submenu);
  submenu.addItem("Back");
  submenu.downOnLast("first");

  do {
    int16_t i = submenu.runOnce();
    if (i == 0) {
      return;
    }

    if (submenu.pickName() == "Start") {
      do_interval(fctx, &interval);
    }

  } while (submenu.pickName() != "Back");
}
