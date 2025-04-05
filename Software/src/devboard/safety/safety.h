#ifndef SAFETY_H
#define SAFETY_H
#include <Arduino.h>
#include <string>
#include "../../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define MAX_CAN_FAILURES 50

#define MAX_CHARGE_DISCHARGE_LIMIT_FAILURES 5

//battery pause status begin
enum battery_pause_status { NORMAL = 0, PAUSING = 1, PAUSED = 2, RESUMING = 3 };
extern bool emulator_pause_request_ON;
extern bool emulator_pause_CAN_send_ON;
extern battery_pause_status emulator_pause_status;
extern bool allowed_to_send_CAN;
//battery pause status end

extern void store_settings_equipment_stop();

void update_machineryprotection();

//battery pause status begin
void setBatteryPause(bool pause_battery, bool pause_CAN, bool equipment_stop = false, bool store_settings = true);
void update_pause_state();
std::string get_emulator_pause_status();
//battery pause status end

#endif
