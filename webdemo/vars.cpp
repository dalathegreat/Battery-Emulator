#include <string>

#include <stdint.h>

/* various settings-y vars */

const char* version_number = "0.0.1";
uint8_t user_selected_can_addon_crystal_frequency_mhz;
uint8_t user_selected_canfd_addon_crystal_frequency_mhz;
int equipment_stop_behavior;
bool use_canfd_as_can;
bool mqtt_enabled;
uint16_t mqtt_timeout_ms;
uint16_t mqtt_publish_interval_ms;
bool ha_autodiscovery_enabled;
bool mqtt_transmit_all_cellvoltages;
std::string mqtt_server;
int mqtt_port;
std::string mqtt_user;
std::string mqtt_password;
