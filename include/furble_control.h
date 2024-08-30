#ifndef FURBLE_CONTROL_H
#define FURBLE_CONTROL_H

#include <Camera.h>

#define CONTROL_CMD_QUEUE_LEN (32)

typedef enum {
  CONTROL_CMD_SHUTTER_PRESS,
  CONTROL_CMD_SHUTTER_RELEASE,
  CONTROL_CMD_FOCUS_PRESS,
  CONTROL_CMD_FOCUS_RELEASE,
  CONTROL_CMD_GPS_UPDATE
} control_cmd_t;

void control_update_gps(Furble::Camera::gps_t &gps, Furble::Camera::timesync_t &timesync);
void control_task(void *param);

#endif
