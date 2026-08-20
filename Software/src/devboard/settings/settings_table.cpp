#include "settings_table.h"

const SettingDesc& setting_desc(Sid sid) {
  return SETTINGS_TABLE[static_cast<size_t>(sid)];
}
