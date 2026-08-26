#include "settings_table.h"

// Not used here - included so the device toolchain compiles the accessor
// header even while its first firmware call sites are still on the way.
#include "settings_accessors.h"

const SettingDesc& setting_desc(Sid sid) {
  return SETTINGS_TABLE[static_cast<size_t>(sid)];
}
