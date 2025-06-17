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

// Wraps the Preferences object begin/end calls, so that the scope of this object
// runs them automatically (via constructor/destructor).
class BatteryEmulatorSettingsStore {
 public:
  BatteryEmulatorSettingsStore(bool readOnly = false) {
    if (!settings.begin("batterySettings", readOnly)) {
      set_event(EVENT_PERSISTENT_SAVE_INFO, 0);
    }
  }

  ~BatteryEmulatorSettingsStore() { settings.end(); }

  uint32_t getUInt(const char* name, uint32_t defaultValue) { return settings.getUInt(name, defaultValue); }

  void saveUInt(const char* name, uint32_t value) { settings.putUInt(name, value); }

  bool getBool(const char* name) { return settings.getBool(name, false); }

  void saveBool(const char* name, bool value) { settings.putBool(name, value); }

 private:
  Preferences settings;
};

#endif
