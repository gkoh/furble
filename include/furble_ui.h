#ifndef FURBLE_UI_H
#define FURBLE_UI_H

#include <Furble.h>

struct FurbleCtx {
  Furble::Device *device;
  bool reconnected;
};

#endif
