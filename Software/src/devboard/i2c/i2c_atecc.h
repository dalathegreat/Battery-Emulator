#ifndef I2C_ATECC_H
#define I2C_ATECC_H

#include <Arduino.h>
#include "../../communication/nvm/comm_nvm.h"

extern bool is_atecc_online;

void init_i2c_atecc(BatteryEmulatorSettingsStore& settings);

#endif