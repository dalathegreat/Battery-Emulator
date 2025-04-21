#ifndef RENAULT_ZOE_GEN2_BATTERY_H
#define RENAULT_ZOE_GEN2_BATTERY_H
#include "../include.h"

#define BATTERY_SELECTED
#define MAX_PACK_VOLTAGE_DV 4100  //5000 = 500.0V
#define MIN_PACK_VOLTAGE_DV 3000
#define MAX_CELL_DEVIATION_MV 150
#define MAX_CELL_VOLTAGE_MV 4250  //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE_MV 2700  //Battery is put into emergency stop if one cell goes below this value

#define POLL_SOC 0x9001
#define POLL_USABLE_SOC 0x9002
#define POLL_SOH 0x9003
#define POLL_PACK_VOLTAGE 0x9005
#define POLL_MAX_CELL_VOLTAGE 0x9007
#define POLL_MIN_CELL_VOLTAGE 0x9009
#define POLL_12V 0x9011
#define POLL_AVG_TEMP 0x9012
#define POLL_MIN_TEMP 0x9013
#define POLL_MAX_TEMP 0x9014
#define POLL_MAX_POWER 0x9018
#define POLL_INTERLOCK 0x901A
#define POLL_KWH 0x91C8
#define POLL_CURRENT 0x925D
#define POLL_CURRENT_OFFSET 0x900C
#define POLL_MAX_GENERATED 0x900E
#define POLL_MAX_AVAILABLE 0x900F
#define POLL_CURRENT_VOLTAGE 0x9130
#define POLL_CHARGING_STATUS 0x9019
#define POLL_REMAINING_CHARGE 0xF45B
#define POLL_BALANCE_CAPACITY_TOTAL 0x924F
#define POLL_BALANCE_TIME_TOTAL 0x9250
#define POLL_BALANCE_CAPACITY_SLEEP 0x9251
#define POLL_BALANCE_TIME_SLEEP 0x9252
#define POLL_BALANCE_CAPACITY_WAKE 0x9262
#define POLL_BALANCE_TIME_WAKE 0x9263
#define POLL_BMS_STATE 0x9259
#define POLL_BALANCE_SWITCHES 0x912B
#define POLL_ENERGY_COMPLETE 0x9210
#define POLL_ENERGY_PARTIAL 0x9215
#define POLL_SLAVE_FAILURES 0x9129
#define POLL_MILEAGE 0x91CF
#define POLL_FAN_SPEED 0x912E
#define POLL_FAN_PERIOD 0x91F4
#define POLL_FAN_CONTROL 0x91C9
#define POLL_FAN_DUTY 0x91F5
#define POLL_TEMPORISATION 0x9281
#define POLL_TIME 0x9261
#define POLL_PACK_TIME 0x91C1
#define POLL_SOC_MIN 0x91B9
#define POLL_SOC_MAX 0x91BA

class RenaultZoeGen2Battery : public CanBattery {
 public:
  RenaultZoeGen2Battery() : CanBattery(RenaultZoeGen2) {}
  virtual const char* name() { return Name; };
  static constexpr char* Name = "Renault Zoe Gen2 50kWh";
  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();

 private:
  uint16_t battery_soc = 0;
  uint16_t battery_usable_soc = 5000;
  uint16_t battery_soh = 10000;
  uint16_t battery_pack_voltage = 370;
  uint16_t battery_max_cell_voltage = 3700;
  uint16_t battery_min_cell_voltage = 3700;
  uint16_t battery_12v = 12000;
  uint16_t battery_avg_temp = 920;
  uint16_t battery_min_temp = 920;
  uint16_t battery_max_temp = 920;
  uint16_t battery_max_power = 0;
  uint16_t battery_interlock = 0;
  uint16_t battery_kwh = 0;
  int32_t battery_current = 32640;
  uint16_t battery_current_offset = 0;
  uint16_t battery_max_generated = 0;
  uint16_t battery_max_available = 0;
  uint16_t battery_current_voltage = 0;
  uint16_t battery_charging_status = 0;
  uint16_t battery_remaining_charge = 0;
  uint16_t battery_balance_capacity_total = 0;
  uint16_t battery_balance_time_total = 0;
  uint16_t battery_balance_capacity_sleep = 0;
  uint16_t battery_balance_time_sleep = 0;
  uint16_t battery_balance_capacity_wake = 0;
  uint16_t battery_balance_time_wake = 0;
  uint16_t battery_bms_state = 0;
  uint16_t battery_balance_switches = 0;
  uint16_t battery_energy_complete = 0;
  uint16_t battery_energy_partial = 0;
  uint16_t battery_slave_failures = 0;
  uint16_t battery_mileage = 0;
  uint16_t battery_fan_speed = 0;
  uint16_t battery_fan_period = 0;
  uint16_t battery_fan_control = 0;
  uint16_t battery_fan_duty = 0;
  uint16_t battery_temporisation = 0;
  uint16_t battery_time = 0;
  uint16_t battery_pack_time = 0;
  uint16_t battery_soc_min = 0;
  uint16_t battery_soc_max = 0;

  CAN_frame ZOE_373 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x373,
                       .data = {0xC1, 0x80, 0x5D, 0x5D, 0x00, 0x00, 0xff, 0xcb}};
  CAN_frame ZOE_POLL_18DADBF1 = {.FD = false,
                                 .ext_ID = true,
                                 .DLC = 8,
                                 .ID = 0x18DADBF1,
                                 .data = {0x03, 0x22, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00}};
  //NVROL Reset
  CAN_frame ZOE_NVROL_1_18DADBF1 = {.FD = false,
                                    .ext_ID = true,
                                    .DLC = 8,
                                    .ID = 0x18DADBF1,
                                    .data = {0x02, 0x10, 0x03, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}};
  CAN_frame ZOE_NVROL_2_18DADBF1 = {.FD = false,
                                    .ext_ID = true,
                                    .DLC = 8,
                                    .ID = 0x18DADBF1,
                                    .data = {0x04, 0x31, 0x01, 0xB0, 0x09, 0x00, 0xAA, 0xAA}};
  //Enable temporisation before sleep
  CAN_frame ZOE_SLEEP_1_18DADBF1 = {.FD = false,
                                    .ext_ID = true,
                                    .DLC = 8,
                                    .ID = 0x18DADBF1,
                                    .data = {0x02, 0x10, 0x03, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}};
  CAN_frame ZOE_SLEEP_2_18DADBF1 = {.FD = false,
                                    .ext_ID = true,
                                    .DLC = 8,
                                    .ID = 0x18DADBF1,
                                    .data = {0x04, 0x2E, 0x92, 0x81, 0x01, 0xAA, 0xAA, 0xAA}};

  const uint16_t poll_commands[48] = {POLL_SOC,
                                      POLL_USABLE_SOC,
                                      POLL_SOH,
                                      POLL_PACK_VOLTAGE,
                                      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                      POLL_MAX_CELL_VOLTAGE,
                                      POLL_MIN_CELL_VOLTAGE,
                                      POLL_12V,
                                      POLL_AVG_TEMP,
                                      POLL_MIN_TEMP,
                                      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                      POLL_MAX_TEMP,
                                      POLL_MAX_POWER,
                                      POLL_INTERLOCK,
                                      POLL_KWH,
                                      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                      POLL_CURRENT_OFFSET,
                                      POLL_MAX_GENERATED,
                                      POLL_MAX_AVAILABLE,
                                      POLL_CURRENT_VOLTAGE,
                                      POLL_CHARGING_STATUS,
                                      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                      POLL_REMAINING_CHARGE,
                                      POLL_BALANCE_CAPACITY_TOTAL,
                                      POLL_BALANCE_TIME_TOTAL,
                                      POLL_BALANCE_CAPACITY_SLEEP,
                                      POLL_BALANCE_TIME_SLEEP,
                                      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                      POLL_BALANCE_CAPACITY_WAKE,
                                      POLL_BALANCE_TIME_WAKE,
                                      POLL_BMS_STATE,
                                      POLL_BALANCE_SWITCHES,
                                      POLL_ENERGY_COMPLETE,
                                      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                      POLL_ENERGY_PARTIAL,
                                      POLL_SLAVE_FAILURES,
                                      POLL_MILEAGE,
                                      POLL_FAN_SPEED,
                                      POLL_FAN_PERIOD,
                                      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                      POLL_FAN_CONTROL,
                                      POLL_FAN_DUTY,
                                      POLL_TEMPORISATION,
                                      POLL_TIME,
                                      POLL_PACK_TIME,
                                      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
                                      POLL_SOC_MIN,
                                      POLL_SOC_MAX};
  uint8_t counter_373 = 0;
  uint8_t poll_index = 0;
  uint16_t currentpoll = POLL_SOC;
  uint16_t reply_poll = 0;

  unsigned long previousMillis200 = 0;  // will store last time a 200ms CAN Message was sent
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was sent
};

#endif
