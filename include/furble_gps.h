#ifndef FURBLE_GPS_H
#define FURBLE_GPS_H

#include <Camera.h>
#include <TinyGPS++.h>

extern TinyGPSPlus furble_gps;

extern bool furble_gps_enable;

void furble_gps_init(void);
void furble_gps_update_geodata(Furble::Camera *camera);

#endif
