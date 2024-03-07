#include <Furble.h>
#include <M5ez.h>

#include "interval.h"
#include "settings.h"
#include "spinner.h"

static interval_t interval;

static bool configure_count(ezMenu *menu) {
  spinner_modify_value("Count", false, &interval.count);
  menu->setCaption("interval_count", "Count\t" + sv2str(&interval.count));
  settings_save_interval(&interval);

  return true;
}

static bool configure_delay(ezMenu *menu) {
  spinner_modify_value("Delay", true, &interval.delay);
  menu->setCaption("interval_delay", "Delay\t" + sv2str(&interval.delay));
  settings_save_interval(&interval);

  return true;
}

static bool configure_shutter(ezMenu *menu) {
  spinner_modify_value("Shutter", true, &interval.shutter);
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

static void display_interval_msg(interval_state_t state, unsigned int count, unsigned int config_count, unsigned long now, unsigned long next) {
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
  char hms[32] = { 0x0 };
  snprintf(hms, 32, "%03u/%03u|%02u:%02u:%02u", count, config_count, rem_h, rem_m, rem_s);
  Serial.println(hms);

  prev_update_ms = now;
  ez.msgBox((String)statestr, (String)hms, "Stop", false);
}

static void do_interval(Furble::Device *device, interval_t *interval) {
  const unsigned int config_count = interval->count.value;
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

    switch (state) {
      case INTERVAL_SHUTTER_OPEN:
        if (icount < config_count) {
          //device->shutterPress();
	  Serial.println("Shutter Open");
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
	//device->shutterRelease();
        Serial.println("Shutter Release");
	next = now + config_delay;
	if (icount < config_count) {
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
        //device->shutterRelease();
	Serial.print("Shutter Release");
      }
      active = false;
    }

    ez.yield();
    display_interval_msg(state, icount, config_count, now, next);
    delay(10);
  } while (active);
  ez.backlight.inactivity(USER_SET);
}

void remote_interval(Furble::Device *device) {
  settings_load_interval(&interval);

  ezMenu submenu(FURBLE_STR " - Interval");
  submenu.buttons("OK#down");
  submenu.addItem("Start");
  submenu.addItem("interval_count | Count\t" + sv2str(&interval.count), NULL, configure_count);
  submenu.addItem("interval_delay | Delay\t" + sv2str(&interval.delay), NULL, configure_delay);
  submenu.addItem("interval_shutter | Shutter\t" + sv2str(&interval.shutter), NULL, configure_shutter);
  submenu.addItem("Back");
  submenu.downOnLast("first");

  do {
    int16_t i = submenu.runOnce();

    if (submenu.pickName() == "Start") {
      do_interval(device, &interval);
    }

    if (i == 0) {
      return;
    }

  } while (submenu.pickName() != "Back");
}
