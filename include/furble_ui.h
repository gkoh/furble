#ifndef FURBLE_UI_H
#define FURBLE_UI_H

#include "furble_control.h"

struct FurbleCtx {
  Furble::Control *control;
  bool cancelled;
};

void vUITask(void *param);

#endif
