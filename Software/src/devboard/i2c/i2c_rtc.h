#ifndef I2C_RTC_H
#define I2C_RTC_H

#include <Arduino.h>
#include "../../communication/nvm/comm_nvm.h"

extern bool is_rtc_online;

void init_i2c_rtc(BatteryEmulatorSettingsStore& settings);
void syncSystemTimeFromRTC();
void updateRTCFromSystemTime();

#endif
