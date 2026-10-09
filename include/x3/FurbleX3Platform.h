#ifndef FURBLE_X3_PLATFORM_H
#define FURBLE_X3_PLATFORM_H

#include <esp_err.h>
#include "x3/FurbleX3App.h"

namespace Furble {
class X3Platform {
 public:
  static esp_err_t init();
  [[noreturn]] static void startupFailure();
  static X3App::Hooks hooks();
};
}  // namespace Furble

#endif
