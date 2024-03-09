#ifndef SETTINGS_H
#define SETTINGS_H

#include <esp_bt.h>
#include <M5ez.h>

#include "interval.h"

extern interval_t interval;

void settings_menu_tx_power(void);
esp_power_level_t settings_load_esp_tx_power(void);
void settings_menu_gps(void);

void settings_load_interval(interval_t *interval);
void settings_save_interval(interval_t *interval);

void settings_add_interval_items(ezMenu *submenu);
void settings_menu_interval(void);

#endif
