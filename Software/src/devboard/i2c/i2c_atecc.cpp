#include "i2c_atecc.h"
#include "../../lib/ArduinoECCX08/ArduinoECCX08.h"
#include "../../lib/ArduinoECCX08/utility/ECCX08DefaultTLSConfig.h"
#include "../../lib/ArduinoECCX08/utility/ECCX08CSR.h"
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

// =======================================================
// ☁️ ATECC608A Cloud Security APIs (Call from WebServer)
// =======================================================

String atecc_get_serial() {
  if (!is_atecc_online) return "NOT_FOUND";
  return ECCX08.serialNumber();
}

bool atecc_is_config_locked() {
  if (!is_atecc_online) return false;
  return ECCX08.locked();
}

bool atecc_is_data_locked() {
  if (!is_atecc_online) return false;
  // Data Zone is usually left chilled during development or for checking key creation status.
  // Initially, link it to Config Lock.
  return ECCX08.locked();
}

bool atecc_lock_config_zone() {
  if (!is_atecc_online) return false;
  if (ECCX08.locked()) return true;

  logging.println("🔐 PROVISIONING: Writing Default TLS Configuration to ATECC608A...");

  // ☢️ [Danger: One-time use only] This command will format channels 0-15 according to TLS standards and permanently lock them!
  bool success = ECCX08.writeConfiguration(ECCX08_DEFAULT_TLS_CONFIG);

  if (success) {
    logging.println("✅ Config Zone LOCKED Successfully!");
  } else {
    logging.println("❌ ERROR: Failed to lock Config Zone.");
  }
  return success;
}

bool atecc_lock_data_zone() {
  if (!is_atecc_online) return false;
  // Simulation Mode: During the development phase, we usually don't lock the Data Zone permanently.
  // Just in case you want to change the certificate in Slot 8 again in the future.
  return true;
}

String atecc_generate_csr(String cn, String platform, String country, String state, String locality, String org) {

  if (!is_atecc_online) return "";

  // The strict rule: The chip must be locked in the Config Zone before a key can be generated!
  if (!ECCX08.locked()) {
    logging.println("❌ ERROR: ATECC is NOT LOCKED. Cannot generate CSR! Please provision first.");
    return "";
  }

  logging.printf("🚀 Generating Private Key in Slot 0 for %s...\n", cn.c_str());

  // 🔑 Create Private Key in Slot 0 (This key will never detach from the chip, ever.)
  if (!ECCX08CSR.begin(0, true)) {
    logging.println("❌ ERROR: Failed to generate Private Key or start CSR.");
    return "";
  }

  // 2. Insert data into the certificate (convert String to const char* using .c_str())

  ECCX08CSR.setCountryName(country.c_str());
  ECCX08CSR.setStateProvinceName(state.c_str());
  ECCX08CSR.setLocalityName(locality.c_str());
  ECCX08CSR.setOrganizationName(org.c_str());
  ECCX08CSR.setOrganizationalUnitName(platform.c_str());
  ECCX08CSR.setCommonName(cn.c_str());

  // 3. Convert message to PEM
  String csr = ECCX08CSR.end();

  if (csr.length() == 0) {
    logging.println("❌ ERROR: Failed to construct CSR.");
  } else {
    logging.println("✅ CSR Generated Successfully!");
  }

  return csr;
}