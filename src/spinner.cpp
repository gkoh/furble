#include <Arduino.h>
#include <M5Unified.h>
#include <M5ez.h>

#include "spinner.h"

static const char *unit2str[5] = {"    ",   // SPIN_UNIT_NIL
                                  "    ",   // SPIN_UNIT_INF
                                  "msec",   // SPIN_UNIT_MS
                                  "secs",   // SPIN_UNIT_SEC
                                  "mins"};  // SPIN_UNIT_MIN

#define PRESET_NUM 10
static const std::array<uint16_t, PRESET_NUM> spin_preset = {1, 2, 4, 8, 15, 30, 60, 125, 250, 500};

#define FMT_NONE_LEN (4)
static const std::array<const char *, FMT_NONE_LEN> fmt_none = {
    " %1u  %1u  %1u ",
    "[%1u] %1u  %1u ",
    " %1u [%1u] %1u ",
    " %1u  %1u [%1u]",
};

#define FMT_UNIT_LEN (5)
static const std::array<const char *, FMT_UNIT_LEN> fmt_unit = {
    " %1u  %1u  %1u  %4s ", "[%1u] %1u  %1u  %4s ", " %1u [%1u] %1u  %4s ",
    " %1u  %1u [%1u] %4s ", " %1u  %1u  %1u [%4s]",
};

#define FMT_PRESET_NONE_LEN (2)
static const std::array<const char *, FMT_PRESET_NONE_LEN> fmt_preset_none = {
    " %1u  %1u  %1u ",
    "[%1u  %1u  %1u]",
};

#define FMT_PRESET_UNIT_LEN (3)
static const std::array<const char *, FMT_PRESET_UNIT_LEN> fmt_preset_unit = {
    " %1u  %1u  %1u  %4s ",
    "[%1u  %1u  %1u] %4s ",
    " %1u  %1u  %1u [%4s]",
};

#define SPIN_ROW_LEN (32)

static void value2htu(uint16_t value, unsigned int *h, unsigned int *t, unsigned int *u) {
  // deconstruct the value
  *h = value / 100;
  *t = (value % 100) / 10;
  *u = value % 10;
}

static uint16_t htu2value(unsigned int h, unsigned int t, unsigned int u) {
  // reconstruct the value
  return (h * 100) + (t * 10) + u;
}

static void display_spinner(const char *title,
                            bool preset,
                            unsigned int index,
                            unsigned int h,
                            unsigned int t,
                            unsigned int u,
                            spin_unit_t unit) {
  char spin_row[SPIN_ROW_LEN] = {0x0};

  if (preset) {
    if (unit == SPIN_UNIT_NIL) {
      snprintf(spin_row, SPIN_ROW_LEN, fmt_preset_none[index], h, t, u);
    } else {
      snprintf(spin_row, SPIN_ROW_LEN, fmt_preset_unit[index], h, t, u, unit2str[unit]);
    }
  } else {
    if (unit == SPIN_UNIT_NIL) {
      snprintf(spin_row, SPIN_ROW_LEN, fmt_none[index], h, t, u);
    } else {
      snprintf(spin_row, SPIN_ROW_LEN, fmt_unit[index], h, t, u, unit2str[unit]);
    }
  }

  std::vector<std::string> buttons = {"Adjust", "Next"};
  if (index == 0) {
    buttons[0] = "OK";
  }

  ez.msgBox(title, {spin_row}, buttons, false);
}

static void spinner_preset(const char *title, SpinValue *sv) {
  const unsigned int imax =
      sv->unit == SPIN_UNIT_NIL ? fmt_preset_none.size() : fmt_preset_unit.size();
  unsigned int i = 0;
  unsigned int n = 0;
  bool ok = false;

  uint16_t value = sv->value;
  unsigned int h;
  unsigned int t;
  unsigned int u;

  // find closest preset equal or lower
  for (n = (spin_preset.size() - 1); n > 0; n--) {
    if (value >= spin_preset[n]) {
      break;
    }
  }

  value2htu(value, &h, &t, &u);
  display_spinner(title, true, i, h, t, u, sv->unit);
  M5.update();

  do {
    if (M5.BtnA.wasClicked()) {
      switch (i) {
        case 0:
          ok = true;
          break;
        case 1:
          n++;
          if (n >= spin_preset.size()) {
            n = 0;
          }
          value = spin_preset[n];
          break;
        case 2:
          switch (sv->unit) {
            case SPIN_UNIT_MS:
              sv->unit = SPIN_UNIT_SEC;
              break;
            case SPIN_UNIT_SEC:
              sv->unit = SPIN_UNIT_MIN;
              break;
            case SPIN_UNIT_MIN:
              sv->unit = SPIN_UNIT_MS;
              break;
            default:
              sv->unit = SPIN_UNIT_NIL;
          }
          break;
      }
      value2htu(value, &h, &t, &u);
      display_spinner(title, true, i, h, t, u, sv->unit);
    }

    if (M5.BtnB.wasClicked()) {
      i++;
      if (i >= imax) {
        i = 0;
      }
      display_spinner(title, true, i, h, t, u, sv->unit);
    }

    ez.yield();
    delay(50);
  } while (!ok);

  // reconstruct the value
  sv->value = htu2value(h, t, u);
}

static void spinner_custom(const char *title, SpinValue *sv) {
  const unsigned int imax = sv->unit == SPIN_UNIT_NIL ? fmt_none.size() : fmt_unit.size();
  unsigned int i = 0;
  bool ok = false;

  unsigned int h = 0;
  unsigned int t = 0;
  unsigned int u = 0;

  value2htu(sv->value, &h, &t, &u);
  display_spinner(title, false, i, h, t, u, sv->unit);
  M5.update();

  do {
    if (M5.BtnA.wasClicked()) {
      switch (i) {
        case 0:
          ok = true;
          break;
        case 1:
          h++;
          if (h > 9) {
            h = 0;
          }
          break;
        case 2:
          t++;
          if (t > 9) {
            t = 0;
          }
          break;
        case 3:
          u++;
          if (u > 9) {
            u = 0;
          }
          break;
        case 4:
          switch (sv->unit) {
            case SPIN_UNIT_MS:
              sv->unit = SPIN_UNIT_SEC;
              break;
            case SPIN_UNIT_SEC:
              sv->unit = SPIN_UNIT_MIN;
              break;
            case SPIN_UNIT_MIN:
              sv->unit = SPIN_UNIT_MS;
              break;
            default:
              sv->unit = SPIN_UNIT_NIL;
          }
          break;
      }
      display_spinner(title, false, i, h, t, u, sv->unit);
    }

    if (M5.BtnB.wasClicked()) {
      i++;
      if (i >= imax) {
        i = 0;
      }
      display_spinner(title, false, i, h, t, u, sv->unit);
    }

    ez.yield();
    delay(50);
  } while (!ok);

  // reconstruct the value
  sv->value = htu2value(h, t, u);
}

void spinner_modify_value(const char *title, bool preset, SpinValue *sv) {
  if (preset) {
    spinner_preset(title, sv);
  } else {
    spinner_custom(title, sv);
  }
}

std::string sv2str(SpinValue *sv) {
  return std::to_string(sv->value) + unit2str[sv->unit];
}

unsigned long sv2ms(SpinValue *sv) {
  switch (sv->unit) {
    case SPIN_UNIT_MIN:
      return (sv->value * 60 * 1000);
    case SPIN_UNIT_SEC:
      return (sv->value * 1000);
    case SPIN_UNIT_MS:
      return (sv->value);
    default:
      return 0;
  }
  return 0;
}

void ms2hms(unsigned long ms, unsigned int *h, unsigned int *m, unsigned int *s) {
  *h = (ms / 1000) / (60 * 60);
  *m = ((ms / 1000) / 60) % 60;
  *s = (ms / 1000) % 60;
}
