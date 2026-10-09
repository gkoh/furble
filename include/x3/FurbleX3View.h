#ifndef FURBLE_X3_VIEW_H
#define FURBLE_X3_VIEW_H

#include <cstdint>

namespace Furble {

/** Copied display state: no pointers into the application or camera list. */
struct X3View {
  enum class Kind : uint8_t { LIST, REMOTE, RUN, INFO };
  static constexpr uint8_t MAX_LINES = 12;
  Kind kind = Kind::LIST;
  char title[40] = {};
  char status[80] = {};
  char lines[MAX_LINES][64] = {};
  char footer[80] = {};
  uint8_t lineCount = 0;
  int8_t selected = -1;
  int16_t batteryPercent = -1;
};

}  // namespace Furble

#endif
