#ifndef TESLA_MODEL_3_BATTERY_H
#define TESLA_MODEL_3_BATTERY_H
#include <Arduino.h>
#include "../../USER_SETTINGS.h"
#include "../devboard/config.h"  // Needed for LED defines
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#define ABSOLUTE_MAX_VOLTAGE \
  4030  // 403.0V,if battery voltage goes over this, charging is not possible (goes into forced discharge)
#define ABSOLUTE_MIN_VOLTAGE 2450  // 245.0V if battery voltage goes under this, discharging further is disabled

// These parameters need to be mapped for the Inverter
extern uint16_t SOC;
extern uint16_t StateOfHealth;
extern uint16_t battery_voltage;
extern uint16_t battery_current;
extern uint16_t capacity_Wh;
extern uint16_t remaining_capacity_Wh;
extern uint16_t max_target_discharge_power;
extern uint16_t max_target_charge_power;
extern uint16_t bms_status;
extern uint16_t bms_char_dis_status;
extern uint16_t stat_batt_power;
extern uint16_t temperature_min;
extern uint16_t temperature_max;
extern uint16_t CANerror;
extern uint16_t cell_max_voltage;
extern uint16_t cell_min_voltage;
extern uint8_t LEDcolor;
// Definitions for BMS status
#define STANDBY 0
#define INACTIVE 1
#define DARKSTART 2
#define ACTIVE 3
#define FAULT 4
#define UPDATING 5

void update_values_tesla_model_3_battery();
void receive_can_tesla_model_3_battery(CAN_frame_t rx_frame);
void send_can_tesla_model_3_battery();
void printFaultCodesIfActive();
void printDebugIfActive(uint8_t symbol, const char* message);
void print_int_with_units(char* header, int value, char* units);
void print_SOC(char* header, int SOC);
uint16_t convert2unsignedInt16(int16_t signed_value);

#endif
