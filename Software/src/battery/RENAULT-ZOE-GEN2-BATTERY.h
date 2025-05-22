#ifndef RENAULT_ZOE_GEN2_BATTERY_H
#define RENAULT_ZOE_GEN2_BATTERY_H
#include "../include.h"

#include "CanBattery.h"

#define BATTERY_SELECTED
#define SELECTED_BATTERY_CLASS RenaultZoeGen2Battery

class RenaultZoeGen2Battery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

 private:
  static const int MAX_PACK_VOLTAGE_DV = 4100;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 3000;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  static const int POLL_SOC = 0x9001;
  static const int POLL_USABLE_SOC = 0x9002;
  static const int POLL_SOH = 0x9003;
  static const int POLL_PACK_VOLTAGE = 0x9005;
  static const int POLL_MAX_CELL_VOLTAGE = 0x9007;
  static const int POLL_MIN_CELL_VOLTAGE = 0x9009;
  static const int POLL_12V = 0x9011;
  static const int POLL_AVG_TEMP = 0x9012;
  static const int POLL_MIN_TEMP = 0x9013;
  static const int POLL_MAX_TEMP = 0x9014;
  static const int POLL_MAX_POWER = 0x9018;
  static const int POLL_INTERLOCK = 0x901A;
  static const int POLL_KWH = 0x91C8;
  static const int POLL_CURRENT = 0x925D;
  static const int POLL_CURRENT_OFFSET = 0x900C;
  static const int POLL_MAX_GENERATED = 0x900E;
  static const int POLL_MAX_AVAILABLE = 0x900F;
  static const int POLL_CURRENT_VOLTAGE = 0x9130;
  static const int POLL_CHARGING_STATUS = 0x9019;
  static const int POLL_REMAINING_CHARGE = 0xF45B;
  static const int POLL_BALANCE_CAPACITY_TOTAL = 0x924F;
  static const int POLL_BALANCE_TIME_TOTAL = 0x9250;
  static const int POLL_BALANCE_CAPACITY_SLEEP = 0x9251;
  static const int POLL_BALANCE_TIME_SLEEP = 0x9252;
  static const int POLL_BALANCE_CAPACITY_WAKE = 0x9262;
  static const int POLL_BALANCE_TIME_WAKE = 0x9263;
  static const int POLL_BMS_STATE = 0x9259;
  static const int POLL_BALANCE_SWITCHES = 0x912B;
  static const int POLL_ENERGY_COMPLETE = 0x9210;
  static const int POLL_ENERGY_PARTIAL = 0x9215;
  static const int POLL_SLAVE_FAILURES = 0x9129;
  static const int POLL_MILEAGE = 0x91CF;
  static const int POLL_FAN_SPEED = 0x912E;
  static const int POLL_FAN_PERIOD = 0x91F4;
  static const int POLL_FAN_CONTROL = 0x91C9;
  static const int POLL_FAN_DUTY = 0x91F5;
  static const int POLL_TEMPORISATION = 0x9281;
  static const int POLL_TIME = 0x9261;
  static const int POLL_PACK_TIME = 0x91C1;
  static const int POLL_SOC_MIN = 0x91B9;
  static const int POLL_SOC_MAX = 0x91BA;

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
  uint16_t temporary_variable = 0;
  uint32_t ZOE_376_time_now_s = 1745452800;  // Initialized to make the battery think it is April 24, 2025
  unsigned long kProductionTimestamp_s =
      1614454107;  // Production timestamp in seconds since January 1, 1970. Production timestamp used: February 25, 2021 at 8:08:27 AM GMT

  CAN_frame ZOE_373 = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x373,
      .data = {0xC1, 0x40, 0x5D, 0xB2, 0x00, 0x01, 0xff,
               0xe3}};  // FIXME: remove if not needed: {0xC1, 0x80, 0x5D, 0x5D, 0x00, 0x00, 0xff, 0xcb}};
  CAN_frame ZOE_376 = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x376,
      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A,
               0x00}};  // fill first 6 bytes with 0's. The first 6 bytes are calculated based on the current time.
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

  unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was sent
  unsigned long previousMillis200 = 0;   // will store last time a 200ms CAN Message was sent
  unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was sent
  /**
 * @brief Transmit CAN frame 0x376
 * 
 * @param[in] void
 * 
 * @return void
 * 
 */
  void transmit_can_frame_376(void);

  /**
 * @brief Reset NVROL, by sending specific frames
 *
 * @param[in] void
 *
 * @return void
 */
  void transmit_reset_nvrol_frames(void);

  /**
 * @brief Wait function
 *
 * @param[in] duration_ms wait duration in ms
 *
 * @return void
 */
  void wait_ms(int duration_ms);
};

#endif
