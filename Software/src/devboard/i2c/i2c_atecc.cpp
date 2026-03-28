#include "i2c_atecc.h"
#include "../../lib/ArduinoECCX08/ArduinoECCX08.h"
#include "../utils/logging.h"
#include "i2c_devices.h"

bool is_atecc_online = false;

void init_i2c_atecc(BatteryEmulatorSettingsStore& settings) {
  if (settings.getBool("I2C_ATECC", false)) {
    if (checkI2CDevicePresence(0x60)) {
      logging.println(" ✅ ATECC608A found at 0x60. Initializing...");

      // Start ATECC
      if (!ECCX08.begin()) {
        logging.println(" ❌ ATECC608A initialization failed! (Check wiring or chip version)");
        is_atecc_online = false;
        return;
      }

      is_atecc_online = true;

      // read Serial Number
      String serialNumber = ECCX08.serialNumber();
      logging.print(" 🔐 ATECC Serial Number: ");
      logging.println(serialNumber);

      // Check the "Safe" status (Lock Status)
      // If the chip is brand new, it will not be locked (Unlocked) yet. We will need to install a key in the future.
      if (!ECCX08.locked()) {
        logging.println(" ⚠️ ATECC Status: UNLOCKED (Brand new chip! Needs provisioning)");
      } else {
        logging.println(" 🔒 ATECC Status: LOCKED (Ready for secure operations)");
      }

    } else {
      logging.println(" ❌ ATECC608A enabled in settings, but NOT FOUND on bus! Bypassing...");
      is_atecc_online = false;
    }
  } else {
    logging.println(" ❌ ATECC608A is DISABLED in settings! Bypassing...");
    is_atecc_online = false;
  }
}
