#include "spinner.h"

static const char *unit2str[4] = {
  "    ", // SPIN_UNIT_NIL
  "msec", // SPIN_UNIT_MS
  "secs", // SPIN_UNIT_SEC
  "mins" }; // SPIN_UNIT_MIN

#define FMT_NONE_LEN (4)
static const char *fmt_none[FMT_NONE_LEN] = {
  " %1u  %1u  %1u ",
  "[%1u] %1u  %1u ",
  " %1u [%1u] %1u ",
  " %1u  %1u [%1u]" };

#define FMT_UNIT_LEN (5)
static const char *fmt_unit[FMT_UNIT_LEN] = {
  " %1u  %1u  %1u  %4s ",
  "[%1u] %1u  %1u  %4s ",
  " %1u [%1u] %1u  %4s ",
  " %1u  %1u [%1u] %4s ",
  " %1u  %1u  %1u [%4s]" };

#define SPIN_ROW_LEN (32)

static void display_spinner(const char *title, unsigned int index, unsigned int h, unsigned int t, unsigned int u, spin_unit_t unit) {
  char spin_row[SPIN_ROW_LEN] = { 0x0 };

  if (unit == SPIN_UNIT_NIL) {
    snprintf(spin_row, SPIN_ROW_LEN, fmt_none[index], h, t, u);
  } else {
    snprintf(spin_row, SPIN_ROW_LEN, fmt_unit[index], h, t, u, unit2str[unit]);
  }

  const char *buttons = "Adjust#Next";
  if (index == 0) {
    buttons = "OK#Next";
  }

  ez.msgBox(title, (String)spin_row, buttons, false);
}

void spinner_modify_value(const char *title, SpinValue *sv) {
  const unsigned int imax = sv->unit == SPIN_UNIT_NIL ? FMT_NONE_LEN : FMT_UNIT_LEN;
  unsigned int i = 0;
  bool ok = false;

  // decontruct the value
  unsigned int h = sv->value / 100;
  unsigned int t = (sv->value % 100) / 10;
  unsigned int u = sv->value % 10;

  display_spinner(title, i, h, t, u, sv->unit);
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
      display_spinner(title, i, h, t, u, sv->unit);
    }

    if (M5.BtnB.wasClicked()) {
      i++;
      if (i >= imax) {
        i = 0;
      }
      display_spinner(title, i, h, t, u, sv->unit);
    }

    ez.yield();
    delay(50);
  } while (!ok);

  // reconstruct the value
  sv->value = (h * 100) + (t * 10) + u;
}

String spinvalue2str(SpinValue *sv) {
  return String(sv->value) + unit2str[sv->unit];
}
