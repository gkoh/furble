#include <Furble.h>
#include <M5ez.h>

#include "interval.h"
#include "spinner.h"

static SpinValue interval_count = { 10, SPIN_UNIT_NIL };
static SpinValue interval_delay = { 10, SPIN_UNIT_SEC };
static SpinValue interval_shutter = { 50, SPIN_UNIT_MS };

static bool configure_count(ezMenu *menu) {
  spinner_modify_value("Count", false, &interval_count);
  menu->setCaption("interval_count", "Count\t" + sv2str(&interval_count));

  return true;
}

static bool configure_delay(ezMenu *menu) {
  spinner_modify_value("Delay", true, &interval_delay);
  menu->setCaption("interval_delay", "Delay\t" + sv2str(&interval_delay));

  return true;
}

static bool configure_shutter(ezMenu *menu) {
  spinner_modify_value("Shutter", true, &interval_shutter);
  menu->setCaption("interval_shutter", "Shutter\t" + sv2str(&interval_shutter));

  return true;
}

static void do_interval(Furble::Device *device, SpinValue *count, SpinValue *idelay, SpinValue *shutter) {
  const unsigned int initial_count = count->value;
  const unsigned long initial_delay = sv2ms(idelay);
  const unsigned long initial_shutter = sv2ms(shutter);

  bool shooting = true;

  do {
    ez.yield();
    delay(10);
  } while (shooting);
  device->focusPress();
  delay(250);
  device->shutterPress();
  delay(50);

  ez.msgBox("Interval Release", String(initial_count), "Stop", false);
  device->shutterRelease();
}

void remote_interval(Furble::Device *device) {
  ezMenu submenu(FURBLE_STR " - Interval");
  submenu.buttons("OK#down");
  submenu.addItem("Start");
  submenu.addItem("interval_count | Count\t" + sv2str(&interval_count), NULL, configure_count);
  submenu.addItem("interval_delay | Delay\t" + sv2str(&interval_delay), NULL, configure_delay);
  submenu.addItem("interval_shutter | Shutter\t" + sv2str(&interval_shutter), NULL, configure_shutter);
  submenu.addItem("Back");
  submenu.downOnLast("first");

  do {
    int16_t i = submenu.runOnce();

    if (submenu.pickName() == "Start") {
      do_interval(device, &interval_count, &interval_delay, &interval_shutter);
    }

    if (i == 0) {
      return;
    }

  } while (submenu.pickName() != "Back");
}
