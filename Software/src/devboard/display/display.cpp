#ifdef SMALL_FLASH_DEVICE

// Don't build the display code on small flash devices to save space

void init_display() {}
void update_display() {}

#else

// A realtime display of battery status and events, using a I2C-connected 128x64
// OLED display based on the SSD1306 driver.

#include "../../datalayer/datalayer.h"
#include "oled_task.h"
#include "epaper_task.h"
//#include "../../communication/nvm/comm_nvm.h" // เรียกใช้ระบบ Settings ใหม่
#include "../../devboard/utils/types.h"      // เรียกใช้ Enum GPIOOPT1

//extern BatteryEmulatorSettingsStore settings; // บอกว่ามีตัวแปร settings รออยู่นะ

bool display_initialized = false;

void init_display() {
  
  // --- ส่วนของ E-Paper (เพิ่มใหม่) ---
  // เรียกใช้งานทันที ไม่ต้องรอ Config จากหน้าเว็บ
  #ifdef HW_LILYGO2CAN
    if (user_selected_display_type == DisplayType::EPAPER_SPI_42) {
      Serial.println("Init Display: Starting E-Paper 4.2...");
      setupEpaper(); 
    }
    else if (user_selected_display_type == DisplayType::OLED_I2C) {
      Serial.println("Display Manager: Starting OLED...");
      setupOLED(); // เรียกฟังก์ชันจาก oled_task.cpp
      return;
    }
  #endif
  // ----------------------------------

  display_initialized = true;

}

void update_display() {
  
  if (!display_initialized) {
    return;
  }

  // อ่านค่าปัจจุบัน
  #ifdef HW_LILYGO2CAN
    if (user_selected_display_type == DisplayType::EPAPER_SPI_42) {
      // อัปเดตจอ E-Paper
      // --- 2. Unit Scaling Logic (แปลงหน่วย) ---
      // Voltage: เก็บเป็น dV (x10) -> ต้องหาร 10.0
      // Current: เก็บเป็น dA (x10) -> ต้องหาร 10.0
      // SOC: เก็บเป็น 0-10000 (0.01%) -> ต้องหาร 100
      
      float voltage_v = datalayer.battery.status.voltage_dV / 10.0f;
      float current_a = datalayer.battery.status.current_dA / 10.0f;
      int soc_percent = datalayer.battery.status.real_soc / 100; // ตัดทศนิยมทิ้งเอาแค่ % เต็ม
      
      // เรียกฟังก์ชันของเรา
      updateEpaperDisplay(voltage_v, current_a, soc_percent, "Running");
      //return;
    }
    else if (user_selected_display_type == DisplayType::OLED_I2C) {
      updateOLED();
    }
  #else
    updateOLED(); // Legacy support
  #endif
  }

#endif
