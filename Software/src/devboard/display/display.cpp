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
#include "../../devboard/utils/types.h"

bool display_initialized = false;

void init_display() {
  
  #ifdef HW_LILYGO2CAN
    // E-Paper 
    if (user_selected_display_type == DisplayType::EPAPER_SPI_42_3C) {
      Serial.println("Init Display: Starting E-Paper 4.2...");
      setupEpaper3C(); 
    }
    else if (user_selected_display_type == DisplayType::EPAPER_SPI_42_BW) {
      Serial.println("Init Display: Starting E-Paper 4.2...");
      setupEpaperBW(); 
    }
    else if (user_selected_display_type == DisplayType::OLED_I2C) {
      Serial.println("Display Manager: Starting OLED...");
      setupOLED(); // func from oled_task.cpp
      // return;
    }
  #endif
  display_initialized = true;
}

void update_display() {
  
  if (!display_initialized) {
    return;
  }

  #ifdef HW_LILYGO2CAN
    if (user_selected_display_type == DisplayType::EPAPER_SPI_42_3C) {
      // E-Paper update
      updateEpaper3CDisplay();
      //return;
    }
    else if (user_selected_display_type == DisplayType::EPAPER_SPI_42_BW) {
      // E-Paper update
      updateEpaperBWDisplay();
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
