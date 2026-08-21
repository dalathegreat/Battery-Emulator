#ifndef _COMM_NVM_H_
#define _COMM_NVM_H_

#include <Preferences.h>
#include <WString.h>
#include <limits>
#include "../../datalayer/datalayer.h"
#include "../../devboard/utils/events.h"
#include "../../devboard/utils/logging.h"
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

void erase_phy_cal_data();

/**
 * @brief Store settings
 *
 * @param[in] void
 *
 * @return void
 */
void store_settings();

void clear_wifi_sta_settings();

// Wraps the Preferences object begin/end calls, so that the scope of this object
// runs them automatically (via constructor/destructor).
class BatteryEmulatorSettingsStore {
 public:
  BatteryEmulatorSettingsStore(bool readOnly = false) : readOnly(readOnly) {
    if (!settings.begin("batterySettings", readOnly)) {
      set_event(EVENT_PERSISTENT_SAVE_INFO, 0);
    }
  }

  ~BatteryEmulatorSettingsStore() { settings.end(); }

  void clearAll() {
    if (readOnly) {
      return;
    }
    settings.clear();
    settingsUpdated = true;
  }

  int32_t getInt(const char* name, int32_t defaultValue) {
    return settings.isKey(name) ? settings.getInt(name, defaultValue) : defaultValue;
  }

  void saveInt(const char* name, int32_t value) {
    if (readOnly) {
      return;
    }
    // isKey() check instead of a sentinel default: saving a value equal to the
    // sentinel into a missing key must not be skipped.
    // A skip must also require the stored TYPE TAG to match. NVS records the type
    // a value was written under, and a typed read returns the caller's default
    // when it does not match - so on a mistagged key getX() answers with the
    // default rather than what is stored. Saving exactly that default then
    // compares equal and the write is skipped, and that write is the only thing
    // that would have repaired the tag, since nvs_set_* replaces an entry of any
    // type. The key stays mistagged and every later read returns the row default.
    //
    // Concretely: a WIFIAPENABLED stored as u32, a user switching the AP off, and
    // a device that boots with the AP on because the false was never written.
    if (!settings.isKey(name) || settings.getType(name) != PT_I32 || getInt(name, 0) != value) {
      settings.putInt(name, value);
      settingsUpdated = true;
    }
  }

  uint32_t getUInt(const char* name, uint32_t defaultValue) {
    return settings.isKey(name) ? settings.getUInt(name, defaultValue) : defaultValue;
  }

  void saveUInt(const char* name, uint32_t value) {
    if (readOnly) {
      return;
    }
    // isKey() check instead of a sentinel default: saving a value equal to the
    // sentinel into a missing key must not be skipped.
    if (!settings.isKey(name) || settings.getType(name) != PT_U32 || getUInt(name, 0) != value) {
      settings.putUInt(name, value);
      settingsUpdated = true;
    }
  }

  bool settingExists(const char* name) { return settings.isKey(name); }

  void removeKey(const char* name) {
    if (readOnly) {
      return;
    }
    if (settings.isKey(name)) {
      settings.remove(name);
      settingsUpdated = true;
    }
  }

  bool getBool(const char* name, bool defaultValue = false) {
    return settings.isKey(name) ? settings.getBool(name, defaultValue) : defaultValue;
  }

  void saveBool(const char* name, bool value) {
    if (readOnly) {
      return;
    }
    // isKey() check: a stored 'false' must not be mistaken for a missing key,
    // or the first save of a false value would be skipped and never persisted.
    if (!settings.isKey(name) || settings.getType(name) != PT_U8 || getBool(name, false) != value) {
      settings.putBool(name, value);
      settingsUpdated = true;
    }
  }

  String getString(const char* name) { return getString(name, ""); }

  String getString(const char* name, const char* defaultValue) {
    return settings.isKey(name) ? settings.getString(name, defaultValue) : String(defaultValue);
  }

  void saveString(const char* name, const char* value) {
    if (readOnly) {
      return;
    }
    // isKey() check: a stored empty string must not be mistaken for a missing
    // key, or the first save of an empty value would be skipped.
    if (!settings.isKey(name) || settings.getType(name) != PT_STR || getString(name, "") != String(value)) {
      settings.putString(name, value);
      settingsUpdated = true;
    }
  }

  // Parses an IP string; returns 0.0.0.0 when missing/malformed (fromString leaves partial bytes on failure).
  IPAddress getIP(const char* name) {
    IPAddress ip;
    if (!ip.fromString(getString(name).c_str())) {
      ip = IPAddress();
    }
    return ip;
  }

  bool were_settings_updated() const { return settingsUpdated; }

#ifdef UNIT_TEST
  // Host tests need the stored TYPE TAG and the number of writes that reached
  // storage: the repair is invisible from the value alone, because a mistagged
  // read and a repaired read can report the same thing.
  PreferenceType stored_type(const char* name) { return settings.getType(name); }
  unsigned storage_writes() const { return settings.writes(); }
#endif

 private:
  Preferences settings;
  const bool readOnly;

  // To track if settings were updated
  bool settingsUpdated = false;
};

#endif
