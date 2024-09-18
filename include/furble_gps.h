#ifndef FURBLE_GPS_H
#define FURBLE_GPS_H

#include <TinyGPS++.h>

#include "furble_control.h"

extern TinyGPSPlus furble_gps;

extern bool furble_gps_enable;

void furble_gps_init(void);
void furble_gps_update(Furble::Control *control);

#endif
