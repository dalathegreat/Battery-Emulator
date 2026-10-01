#pragma once

#include <Arduino.h>  // String
#include <string>

// User-configured hostname. Loaded from NVM ("HOSTNAME"); empty when unset
extern std::string custom_hostname;

// Returns the default hostname ("battery-emulator-" + last two bytes of the MAC,
// lowercase) used when no custom hostname is configured.
String default_hostname();

// Returns the effective hostname: the user's custom_hostname if set, otherwise
// default_hostname().
String active_hostname();
