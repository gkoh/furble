#ifndef SETTINGS_H
#define SETTINGS_H

#include <M5ez.h>
#include <esp_bt.h>

#include "interval.h"

void settings_menu_tx_power(void);
esp_power_level_t settings_load_esp_tx_power(void);

bool settings_load_reconnect(void);
void settings_save_reconnect(bool reconnect);

bool settings_load_gps(void);
void settings_menu_gps(void);

void settings_load_interval(interval_t &interval);
void settings_save_interval(const interval_t &interval);

void settings_add_interval_items(ezMenu *submenu, interval_t &interval);
void settings_menu_interval(void);

bool settings_load_multiconnect(void);
void settings_save_multiconnect(bool multiconnect);

#endif
