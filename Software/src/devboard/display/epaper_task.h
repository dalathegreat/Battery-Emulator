#ifndef EPAPER_TASK_H
#define EPAPER_TASK_H

#include <Arduino.h>

// ประกาศชื่อฟังก์ชัน เพื่อให้ไฟล์อื่นเรียกใช้ได้
void setupEpaper();
void updateEpaperDisplay(float voltage, float current, int soc, String status);

// ถ้าจะทำ Task แยก ก็ประกาศตรงนี้ด้วย (ถ้าเลือกวิธี Task)
void task_update_epaper(void *pvParameters);

#endif