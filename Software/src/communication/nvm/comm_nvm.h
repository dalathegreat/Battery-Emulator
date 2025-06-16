#ifndef _COMM_NVM_H_
#define _COMM_NVM_H_

#include "../../include.h"

#include "../../datalayer/datalayer.h"
#include "../../devboard/utils/events.h"
#include "../../devboard/wifi/wifi.h"

/**
 * @brief Initialization of setting storage
 *
 * @param[in] void
 *
 * @return void
 */
void init_stored_settings();

/**
 * @brief Store settings of equipment stop button
 *
 * @param[in] void
 *
 * @return void
 */
void store_settings_equipment_stop();

/**
 * @brief Store settings
 *
 * @param[in] void
 *
 * @return void
 */
void store_settings();

class BatteryEmulatorSettingsStore {
 public:
  BatteryEmulatorSettingsStore() { settings.begin("batterySettings", false); }

  ~BatteryEmulatorSettingsStore() { settings.end(); }

  BatteryType get_batterytype() { return (BatteryType)settings.getUInt("BATTTYPE", (int)BatteryType::None); }

  void set_batterytype(BatteryType type) { settings.putUInt("BATTTYPE", (int)type); }

  InverterProtocolType get_invertertype() {
    return (InverterProtocolType)settings.getUInt("INVTYPE", (int)InverterProtocolType::None);
  }

  void set_invertertype(InverterProtocolType type) { settings.putUInt("INVTYPE", (int)type); }

 private:
  Preferences settings;
};

#endif
