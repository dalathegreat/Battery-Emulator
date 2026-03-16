#ifndef I2C_SHT30_H
#define I2C_SHT30_H

#include <Arduino.h>
#include "../../communication/nvm/comm_nvm.h"

extern bool is_sht30_online;

void init_i2c_sht30(BatteryEmulatorSettingsStore& settings);

#endif