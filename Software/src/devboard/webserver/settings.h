#pragma once

#include "../../lib/bblanchon-ArduinoJson/ArduinoJson.h"

class BatteryEmulatorSettingsStore;  // Defined in comm_nvm.h

void apply_setting_updates(const JsonDocument& doc, JsonDocument& errors, BatteryEmulatorSettingsStore& settings,
                           bool save, bool& reboot_required_saved);

void build_settings_json(JsonDocument& doc, BatteryEmulatorSettingsStore& settings);

// Boot-time load of every table-driven setting (NVM -> live variable).
// Called from init_stored_settings() during boot only.
void load_stored_settings(BatteryEmulatorSettingsStore& settings);

// Persists the runtime-mutable ("Instant") settings: live variable -> NVM.
// Called from store_settings().
void store_settings_from_live(BatteryEmulatorSettingsStore& settings);
