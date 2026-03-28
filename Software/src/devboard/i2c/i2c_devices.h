#ifndef I2C_DEVICES_H
#define I2C_DEVICES_H

#include <Arduino.h>
#include <Wire.h>
#include "../../communication/nvm/comm_nvm.h"
// #include "../debug/logging.h"                 /

// Global variable like extern for public access
extern bool is_sht30_online;
extern bool is_atecc_online;
extern bool is_rtc_online;
extern bool is_io_online;

void setupMultipleI2CDevices(BatteryEmulatorSettingsStore& settings);
bool checkI2CDevicePresence(uint8_t address);

void syncSystemTimeFromRTC();
void updateRTCFromSystemTime();
void init_i2c_bus();
void scan_i2c_bus();

#endif
