#ifdef SMALL_FLASH_DEVICE

// Don't build the display code on small flash devices to save space
void init_display() {}
void update_display() {}

#else

// A realtime display of battery status and events.
// Acts as a dispatcher for OLED (SSD1306) and E-Paper displays.

#include "../../datalayer/datalayer.h"
#include "oled_task.h"
#include "epaper_task.h"
#include "../../devboard/utils/types.h"

bool display_initialized = false;

void init_display() {
  
  #ifdef HW_LILYGO2CAN
    // ----------------------------------------------------
    // T-2CAN Display Dispatcher
    // ----------------------------------------------------
    if (user_selected_display_type == DisplayType::EPAPER_SPI_42_3C) {
      Serial.println("Init Display: Starting E-Paper 4.2\" (3-Color)...");
      setupEpaper3C(); 
    }
    else if (user_selected_display_type == DisplayType::EPAPER_SPI_42_BW) {
      Serial.println("Init Display: Starting E-Paper 4.2\" (B/W)...");
      setupEpaperBW(); 
    }
    else if (user_selected_display_type == DisplayType::OLED_I2C) {
      Serial.println("Init Display: Starting OLED...");
      setupOLED(); 
    }
  #else
    // ----------------------------------------------------
    // Legacy Boards Support (LilyGo V1, STARK, etc.)
    // ----------------------------------------------------
    Serial.println("Init Display: Starting OLED (Legacy)...");
    setupOLED();
  #endif

  display_initialized = true;
}

void update_display() {
  
  if (!display_initialized) {
    return;
  }

  #ifdef HW_LILYGO2CAN
    if (user_selected_display_type == DisplayType::EPAPER_SPI_42_3C) {
      updateEpaper3CDisplay();
    }
    else if (user_selected_display_type == DisplayType::EPAPER_SPI_42_BW) {
      updateEpaperBWDisplay();
    }
    else if (user_selected_display_type == DisplayType::OLED_I2C) {
      updateOLED();
    }
  #else
    updateOLED(); // Legacy boards default to OLED
  #endif
}

#endif