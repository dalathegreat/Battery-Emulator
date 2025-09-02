#include "USER_SETTINGS.h"
#include <string>
#include "src/devboard/hal/hal.h"

// Set your Static IP address. Only used incase WIFICONFIG is set in USER_SETTINGS.h
IPAddress local_IP(192, 168, 10, 150);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);
