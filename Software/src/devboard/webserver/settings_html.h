#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <string>

extern std::string ssid;
extern std::string password;

#include "../../communication/nvm/comm_nvm.h"

/**
 * @brief Replaces placeholder with content section in web page
 *
 * @param[in] var
 *
 * @return String
 */
String settings_processor(const String& var, BatteryEmulatorSettingsStore& settings);
/**
 * @brief Maps the value to a string of characters
 *
 * @param[in] char
 *
 * @return String
 */
const char* getCANInterfaceName(CAN_Interface interface);

extern const char settings_html[];

#endif
