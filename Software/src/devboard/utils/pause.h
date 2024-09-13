#ifndef _PAUSE_H_
#define _PAUSE_H_

#include <string>


//battery pause status
enum battery_pause_status { NORMAL = 0, PAUSING = 1, PAUSED = 2, RESUMING = 3 };
extern bool emulator_pause_request_ON;
extern bool emulator_pause_CAN_send_ON;
extern battery_pause_status emulator_pause_status;
extern bool can_send_CAN;

void setBatteryPause(bool pause_battery,bool pause_CAN) ;
void emulator_pause_state_send_CAN_battery();
std::string get_emulator_pause_status();
#endif