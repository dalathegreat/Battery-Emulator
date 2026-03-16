#include "i2c_devices.h"
#include <src/lib/RTClib/RTClib.h>
#include <sys/time.h>
#include "i2c_atecc.h"
#include "i2c_rtc.h"
#include "i2c_sht30.h"
// #include "i2c_sht30.h"
// #include "i2c_atecc.h"

bool is_io_online = false;

bool checkI2CDevicePresence(uint8_t address) {
  Wire.beginTransmission(address);
  byte error = Wire.endTransmission();
  return (error == 0);  // 0 = found device
}

void setupMultipleI2CDevices(BatteryEmulatorSettingsStore& settings) {
  if (!settings.getBool("MULTII2C", false)) {
    return;  // jump over Multi I2C
  }

  logging.println("🔍 Scanning for active I2C Devices...");

  init_i2c_rtc(settings);
  init_i2c_sht30(settings);
  init_i2c_atecc(settings);

  // 🔌 4. Check PCF8574 IO Expander (0x20)
  if (settings.getBool("I2C_IO", false)) {
    if (checkI2CDevicePresence(0x20)) {
      logging.println(" ✅ PCF8574 IO Expander found at 0x20. Initializing...");
      is_io_online = true;
      // TODO: pcf8574.begin()
    } else {
      logging.println(" ❌ PCF8574 enabled in settings, but NOT FOUND on bus! Bypassing...");
      is_io_online = false;
    }
  }

  logging.println("🏁 I2C Device Scan Completed.");
}
