#ifndef I2C_ATECC_H
#define I2C_ATECC_H

#include <Arduino.h>
#include "../../communication/nvm/comm_nvm.h"

extern bool is_atecc_online;

void init_i2c_atecc(BatteryEmulatorSettingsStore& settings);

// =======================================================
// ☁️ ATECC608A Cloud Security APIs
// =======================================================
String atecc_get_serial();
bool atecc_is_config_locked();
bool atecc_is_data_locked();
String atecc_generate_csr(String cn, String platform, String country, String state, String locality, String org);
bool atecc_lock_config_zone();
bool atecc_lock_data_zone();
//bool atecc_unlock_config_zone();
//bool atecc_unlock_data_zone();

#endif
