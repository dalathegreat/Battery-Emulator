#ifndef IMIEV_CZERO_ION_BATTERY_H
#define IMIEV_CZERO_ION_BATTERY_H
#include <Arduino.h>
#include "../../USER_SETTINGS.h"
#include "../devboard/config.h"  // Needed for all defines
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define ABSOLUTE_MAX_VOLTAGE \
  4040  // 404.4V,if battery voltage goes over this, charging is not possible (goes into forced discharge)
#define ABSOLUTE_MIN_VOLTAGE 3100  // 310.0V if battery voltage goes under this, discharging further is disabled

// These parameters need to be mapped for the Gen24
extern uint16_t SOC;
extern uint16_t StateOfHealth;
extern uint16_t battery_voltage;
extern uint16_t battery_current;
extern uint16_t capacity_Wh;
extern uint16_t remaining_capacity_Wh;
extern uint16_t max_target_discharge_power;
extern uint16_t max_target_charge_power;
extern uint8_t bms_char_dis_status;
extern uint16_t stat_batt_power;
extern uint16_t temperature_min;
extern uint16_t temperature_max;
extern uint16_t CANerror;
extern uint16_t cell_max_voltage;
extern uint16_t cell_min_voltage;
extern bool batteryAllowsContactorClosing;   //Bool, 1=true, 0=false
extern bool inverterAllowsContactorClosing;  //Bool, 1=true, 0=false

void update_values_imiev_battery();
void receive_can_imiev_battery(CAN_frame_t rx_frame);
void send_can_imiev_battery();

#endif
