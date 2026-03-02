#ifndef EPAPER_TASK_H
#define EPAPER_TASK_H

#include <Arduino.h>

// For 3 color, Black, write and Red
void setupEpaper3C();
void updateEpaper3CDisplay();
// For Black and write
void setupEpaperBW();
void updateEpaperBWDisplay();
//void task_update_epaper(void *pvParameters);

#endif