#ifndef FURBLE_UI_H
#define FURBLE_UI_H

#include <Camera.h>

struct FurbleCtx {
  Furble::Camera *camera;
  bool reconnected;
};

void vUITask(void *param);

#endif
