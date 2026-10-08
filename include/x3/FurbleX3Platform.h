#pragma once

#include <esp_err.h>
#include "x3/FurbleX3App.h"

namespace Furble {
class X3Platform {
 public:
  static esp_err_t init();
  static X3App::Hooks hooks();
};
}  // namespace Furble
