#include "i2c_sht30.h"
#include "i2c_devices.h"    
// #include "../debug/logging.h"

// TODO: Include Library SHT30 
// #include <Adafruit_SHT31.h> 

bool is_sht30_online = false;

// Adafruit_SHT31 sht31 = Adafruit_SHT31();

void init_i2c_sht30(BatteryEmulatorSettingsStore& settings) {
  if (settings.getBool("I2C_SHT30", false)) {
    if (checkI2CDevicePresence(0x44)) {
      logging.println(" ✅ SHT30 found at 0x44. Initializing...");
      is_sht30_online = true;
      
      // TODO: Something with sht31.begin(0x44) 
      
    } else {
      logging.println(" ❌ SHT30 enabled in settings, but NOT FOUND on bus! Bypassing...");
      is_sht30_online = false;
    }
  }
}