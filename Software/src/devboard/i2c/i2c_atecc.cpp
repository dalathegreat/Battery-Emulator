#include "i2c_atecc.h"
#include "i2c_devices.h"
// #include "../debug/logging.h"

// TODO: Include Library ATECC
// #include <ArduinoECCX08.h>

bool is_atecc_online = false;

void init_i2c_atecc(BatteryEmulatorSettingsStore& settings) {
  if (settings.getBool("I2C_ATECC", false)) {
    if (checkI2CDevicePresence(0x60)) {
      logging.println(" ✅ ATECC608A found at 0x60. Initializing...");
      is_atecc_online = true;

      // TODO: Coding ECCX08.begin()

    } else {
      logging.println(" ❌ ATECC608A enabled in settings, but NOT FOUND on bus! Bypassing...");
      is_atecc_online = false;
    }
  }
}
