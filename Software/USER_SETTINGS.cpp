#include "USER_SETTINGS.h"
#include <string>
#include "src/devboard/hal/hal.h"

// Set your Static IP address. Only used incase WIFICONFIG is set in USER_SETTINGS.h
IPAddress local_IP(192, 168, 10, 150);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);

/* Charger settings (Optional, when using generator charging) */
volatile float CHARGER_SET_HV = 384;      // Reasonably appropriate 4.0v per cell charging of a 96s pack
volatile float CHARGER_MAX_HV = 420;      // Max permissible output (VDC) of charger
volatile float CHARGER_MIN_HV = 200;      // Min permissible output (VDC) of charger
volatile float CHARGER_MAX_POWER = 3300;  // Max power capable of charger, as a ceiling for validating config
volatile float CHARGER_MAX_A = 11.5;      // Max current output (amps) of charger
volatile float CHARGER_END_A = 1.0;       // Current at which charging is considered complete
