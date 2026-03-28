#include "i2c_devices.h"
#include <Wire.h>
#include <src/lib/RTClib/RTClib.h>
#include <sys/time.h>
#include "../hal/hal.h"
#include "i2c_atecc.h"
#include "i2c_rtc.h"
#include "i2c_sht30.h"
// #include "i2c_sht30.h"
// #include "i2c_atecc.h"

bool is_io_online = false;
bool is_i2c_bus_initialized = false;

bool checkI2CDevicePresence(uint8_t address) {
  Wire.beginTransmission(address);
  byte error = Wire.endTransmission();
  return (error == 0);  // 0 = found device
}

void scan_i2c_bus() {
  byte error, address;
  int nDevices = 0;

  Serial.println("=================================");
  Serial.println("🔍 I2C device scan start...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("✅ Found I2C at Address: 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    } else if (error == 4) {
      Serial.print("⚠️ Got Error at Address: 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (nDevices == 0) {
    Serial.println("❌ Not found I2C! (Please check wire)");
  } else {
    Serial.println("🏁 Scan completed!");
  }
  Serial.println("=================================\n");
}

void init_i2c_bus() {
  if (!is_i2c_bus_initialized) {
    int sda_pin = esp32hal->I2C_SDA_PIN();
    int scl_pin = esp32hal->I2C_SCL_PIN();

    Wire.begin(sda_pin, scl_pin);
    Wire.setClock(400000);  // 400kHz is the most stable speed for connecting multiple devices.
    is_i2c_bus_initialized = true;
    Serial.println("🌐 Central I2C Bus Initialized (400kHz)");
  }
}

void setupMultipleI2CDevices(BatteryEmulatorSettingsStore& settings) {
  // if (!settings.getBool("MULTII2C", false)) {
  //   Serial.println("❌ MULTII2C is DISABLED in settings! Bypassing...");
  //   return;  // jump over Multi I2C
  // }

  if (esp32hal->I2C_SDA_PIN() == GPIO_NUM_NC || esp32hal->I2C_SCL_PIN() == GPIO_NUM_NC) {
    return;
  }

  init_i2c_bus();

  logging.println("🔍 Scanning for active I2C Devices...");
  scan_i2c_bus();

  init_i2c_atecc(settings);
  init_i2c_rtc(settings);
  init_i2c_sht30(settings);

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
