#include "USER_SETTINGS.h"

#ifdef WEBSERVER
#define ENABLE_AP                                     //Comment out this line to turn off the broadcasted AP
const char* ssid = "REPLACE_WITH_YOUR_SSID";          // maximum of 63 characters;
const char* password = "REPLACE_WITH_YOUR_PASSWORD";  // minimum of 8 characters;
const char* ssidAP = "Battery Emulator";              // maximum of 63 characters;
const char* passwordAP = "123456789";  // minimum of 8 characters; set to NULL if you want the access point to be open
#endif
