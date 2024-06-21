#ifndef FURBLE_GPS_H
#define FURBLE_GPS_H

#include <Camera.h>
#include <TinyGPS++.h>
#include <spinner.h>

extern TinyGPSPlus furble_gps;

extern bool furble_gps_enable;
extern SpinValue furble_gps_interval;

void furble_gps_init(void);
void furble_gps_update_geodata(Furble::Camera *camera);
uint16_t furble_gps_update_service(void *private_data);

#endif
