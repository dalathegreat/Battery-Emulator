#ifndef SETTINGS_HTML_H
#define SETTINGS_HTML_H

#include <Arduino.h>

class BatteryEmulatorSettingsStore;

String settings_processor(const String& var, BatteryEmulatorSettingsStore& settings);

extern const char settings_batt_html[];
extern const char settings_net_html[];
extern const char settings_hw_html[];
extern const char settings_web_html[];
extern const char settings_overrides_html[];

#endif