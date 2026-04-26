#ifndef ATTO_3_BATTERY_H
#define ATTO_3_BATTERY_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"

#include "BYD-ATTO-3-HTML.h"
#include "CanBattery.h"

// Ramp down settings that are used when SOC is estimated from voltage
static const int RAMPDOWN_SOC = 100;  // SOC to start ramping down from. Value set here is scaled by 10 (100 = 10.0%)
static const int RAMPDOWN_POWER_ALLOWED =
    10000;  // Power to start ramp down from, set a lower value to limit the power even further as SOC decreases

class BydAttoBattery : public CanBattery {
 public:
  // Use this constructor for the second battery.
  BydAttoBattery(DATALAYER_BATTERY_TYPE* datalayer_ptr, DATALAYER_INFO_BYDATTO3* extended, CAN_Interface targetCan)
      : CanBattery(targetCan), renderer(extended) {
    datalayer_battery = datalayer_ptr;
    datalayer_bydatto = extended;
    allows_contactor_closing = nullptr;
  }

  // Use the default constructor to create the first or single battery.
  BydAttoBattery() : renderer(&datalayer_extended.bydAtto3) {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
    datalayer_bydatto = &datalayer_extended.bydAtto3;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  static constexpr const char* Name = "BYD Atto 3/Seal/Dolphin";

  bool supports_charged_energy() { return true; }
  bool supports_reset_crash() { return true; }
  void reset_crash() { datalayer_bydatto->UserRequestCrashReset = true; }
  bool supports_calibrate_SOC() { return true; }
  void reset_SOC() { datalayer_bydatto->UserRequestCalibrateSOC = true; }

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  BydAtto3HtmlRenderer renderer;
  DATALAYER_BATTERY_TYPE* datalayer_battery;
  DATALAYER_INFO_BYDATTO3* datalayer_bydatto;
  bool* allows_contactor_closing;

  unsigned long previousMillis50 = 0;   // will store last time a 50ms CAN Message was send
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis200 = 0;  // will store last time a 200ms CAN Message was send
  uint32_t last_auto_calibrate_ms = 0;  // Cooldown timer for auto-calibration
  uint32_t tail_dwell_start_ms = 0;     // Tracks when sustained tail current began

  static const int POLL_TIMES_FULL_POWER = 0x0004;  // Using Carscanner name for now.
  static const int POLL_FOR_BATTERY_SOC = 0x0005;
  static const int POLL_FOR_BATTERY_VOLTAGE = 0x0008;
  static const int POLL_FOR_BATTERY_CURRENT = 0x0009;
  static const int POLL_MAX_CHARGE_POWER = 0x000A;
  static const int POLL_CHARGE_TIMES = 0x000B;  // Using Carscanner name for now.
  static const int POLL_MAX_DISCHARGE_POWER = 0x000E;
  static const int POLL_TOTAL_CHARGED_AH = 0x000F;
  static const int POLL_TOTAL_DISCHARGED_AH = 0x0010;
  static const int POLL_TOTAL_CHARGED_KWH = 0x0011;
  static const int POLL_TOTAL_DISCHARGED_KWH = 0x0012;
  static const int POLL_MIN_CELL_VOLTAGE_NUMBER = 0x002A;
  static const int POLL_FOR_BATTERY_CELL_MV_MIN = 0x002B;
  static const int POLL_MAX_CELL_VOLTAGE_NUMBER = 0x002C;
  static const int POLL_FOR_BATTERY_CELL_MV_MAX = 0x002D;
  static const int POLL_MIN_TEMP_MODULE_NUMBER = 0x002E;
  static const int POLL_FOR_LOWEST_TEMP_CELL = 0x002F;
  static const int POLL_MAX_TEMP_MODULE_NUMBER = 0x0030;
  static const int POLL_FOR_HIGHEST_TEMP_CELL = 0x0031;
  static const int POLL_FOR_BATTERY_PACK_AVG_TEMP = 0x0032;
  static const int POLL_MODULE_1_LOWEST_MV_NUMBER = 0x016C;
  static const int POLL_MODULE_1_LOWEST_CELL_MV = 0x016D;
  static const int POLL_MODULE_1_HIGHEST_MV_NUMBER = 0x016E;
  static const int POLL_MODULE_1_HIGH_CELL_MV = 0x016F;
  static const int POLL_MODULE_1_HIGH_TEMP = 0x0171;
  static const int POLL_MODULE_1_LOW_TEMP = 0x0173;
  static const int POLL_MODULE_2_LOWEST_MV_NUMBER = 0x0174;
  static const int POLL_MODULE_2_LOWEST_CELL_MV = 0x0175;
  static const int POLL_MODULE_2_HIGHEST_MV_NUMBER = 0x0176;
  static const int POLL_MODULE_2_HIGH_CELL_MV = 0x0177;
  static const int POLL_MODULE_2_HIGH_TEMP = 0x0179;
  static const int POLL_MODULE_2_LOW_TEMP = 0x017B;
  static const int POLL_MODULE_3_LOWEST_MV_NUMBER = 0x017C;
  static const int POLL_MODULE_3_LOWEST_CELL_MV = 0x017D;
  static const int POLL_MODULE_3_HIGHEST_MV_NUMBER = 0x017E;
  static const int POLL_MODULE_3_HIGH_CELL_MV = 0x017F;
  static const int POLL_MODULE_3_HIGH_TEMP = 0x0181;
  static const int POLL_MODULE_3_LOW_TEMP = 0x0183;
  static const int POLL_MODULE_4_LOWEST_MV_NUMBER = 0x0184;
  static const int POLL_MODULE_4_LOWEST_CELL_MV = 0x0185;
  static const int POLL_MODULE_4_HIGHEST_MV_NUMBER = 0x0186;
  static const int POLL_MODULE_4_HIGH_CELL_MV = 0x0187;
  static const int POLL_MODULE_4_HIGH_TEMP = 0x0189;
  static const int POLL_MODULE_4_LOW_TEMP = 0x018B;
  static const int POLL_MODULE_5_LOWEST_MV_NUMBER = 0x018C;
  static const int POLL_MODULE_5_LOWEST_CELL_MV = 0x018D;
  static const int POLL_MODULE_5_HIGHEST_MV_NUMBER = 0x018E;
  static const int POLL_MODULE_5_HIGH_CELL_MV = 0x018F;
  static const int POLL_MODULE_5_HIGH_TEMP = 0x0191;
  static const int POLL_MODULE_5_LOW_TEMP = 0x0193;
  static const int POLL_MODULE_6_LOWEST_MV_NUMBER = 0x0194;
  static const int POLL_MODULE_6_LOWEST_CELL_MV = 0x0195;
  static const int POLL_MODULE_6_HIGHEST_MV_NUMBER = 0x0196;
  static const int POLL_MODULE_6_HIGH_CELL_MV = 0x0197;
  static const int POLL_MODULE_6_HIGH_TEMP = 0x0199;
  static const int POLL_MODULE_6_LOW_TEMP = 0x019B;
  static const int POLL_MODULE_7_LOWEST_MV_NUMBER = 0x019C;
  static const int POLL_MODULE_7_LOWEST_CELL_MV = 0x019D;
  static const int POLL_MODULE_7_HIGHEST_MV_NUMBER = 0x019E;
  static const int POLL_MODULE_7_HIGH_CELL_MV = 0x019F;
  static const int POLL_MODULE_7_HIGH_TEMP = 0x01A1;
  static const int POLL_MODULE_7_LOW_TEMP = 0x01A3;
  static const int POLL_MODULE_8_LOWEST_MV_NUMBER = 0x01A4;
  static const int POLL_MODULE_8_LOWEST_CELL_MV = 0x01A5;
  static const int POLL_MODULE_8_HIGHEST_MV_NUMBER = 0x01A6;
  static const int POLL_MODULE_8_HIGH_CELL_MV = 0x01A7;
  static const int POLL_MODULE_8_HIGH_TEMP = 0x01A9;
  static const int POLL_MODULE_8_LOW_TEMP = 0x01AB;
  static const int POLL_MODULE_9_LOWEST_MV_NUMBER = 0x01AC;
  static const int POLL_MODULE_9_LOWEST_CELL_MV = 0x01AD;
  static const int POLL_MODULE_9_HIGHEST_MV_NUMBER = 0x01AE;
  static const int POLL_MODULE_9_HIGH_CELL_MV = 0x01AF;
  static const int POLL_MODULE_9_HIGH_TEMP = 0x01B1;
  static const int POLL_MODULE_9_LOW_TEMP = 0x01B3;
  static const int POLL_MODULE_10_LOWEST_MV_NUMBER = 0x01B4;
  static const int POLL_MODULE_10_LOWEST_CELL_MV = 0x01B5;
  static const int POLL_MODULE_10_HIGHEST_MV_NUMBER = 0x01B6;
  static const int POLL_MODULE_10_HIGH_CELL_MV = 0x01B7;
  static const int POLL_MODULE_10_HIGH_TEMP = 0x01B9;
  static const int POLL_MODULE_10_LOW_TEMP = 0x01BB;
  static const int POLL_FOR_ORIGINAL_CALIBRATION = 0x1FFE;
  static const int POLL_FOR_CURRENT_CALIBRATION = 0x1FFC;

  static const uint16_t MAX_CELL_DEVIATION_MV = 230;
  static const uint16_t MAX_CELL_VOLTAGE_MV = 3650;  //Charging stops if one cell exceeds this value
  static const uint16_t MIN_CELL_VOLTAGE_MV = 2800;  //Discharging stops if one cell goes below this value

  uint16_t rampdown_power = 0;
  uint16_t poll_state = POLL_FOR_BATTERY_SOC;
  uint16_t pid_reply = 0;
  uint16_t battery_voltage = 0;
  uint16_t battery_highprecision_SOC = 0;
  uint16_t battery_estimated_SOC = 0;
  uint16_t BMS_SOC = 0;
  uint16_t BMS_voltage = 0;
  uint16_t BMS_lowest_cell_voltage_mV = 3300;
  uint16_t BMS_highest_cell_voltage_mV = 3300;
  uint16_t BMS_allowed_charge_power = 0;
  uint16_t BMS_charge_times = 0;
  uint16_t BMS_allowed_discharge_power = 0;
  uint16_t BMS_total_charged_ah = 0;
  uint16_t BMS_total_discharged_ah = 0;
  uint16_t BMS_total_charged_kwh = 0;
  uint16_t BMS_total_discharged_kwh = 0;
  uint16_t BMS_times_full_power = 0;
  uint16_t BMS_capacity_original_calibration = 0;
  uint16_t BMC_SOC_original_calibration = 0;
  uint16_t BMS_capacity_current_calibration = 0;
  uint16_t BMC_SOC_current_calibration = 0;
  uint16_t seed = 0;
  uint16_t solvedKey = 0;

  int16_t battery_temperature_ambient = 0;
  int16_t battery_lowest_temperature = 0;
  int16_t battery_highest_temperature = 0;
  int16_t battery_calc_min_temperature = 0;
  int16_t battery_calc_max_temperature = 0;
  int16_t BMS_current = 0;
  int16_t BMS_lowest_cell_temperature = 0;
  int16_t BMS_highest_cell_temperature = 0;
  int16_t BMS_average_cell_temperature = 0;

  static const uint8_t NOT_DETERMINED_YET = 0;
  static const uint8_t STANDARD_RANGE = 1;
  static const uint8_t EXTENDED_RANGE = 2;
  static const uint8_t NOT_RUNNING = 0xFF;
  static const uint8_t STARTED = 0;
  static const uint8_t RUNNING_STEP_1 = 1;
  static const uint8_t RUNNING_STEP_2 = 2;
  static const uint8_t RUNNING_STEP_3 = 3;
  uint8_t battery_type = NOT_DETERMINED_YET;
  uint8_t stateMachineClearCrash = NOT_RUNNING;
  uint8_t stateMachineCalibrateSOC = NOT_RUNNING;
  uint8_t counter_50ms = 0;
  uint8_t counter_100ms = 0;
  uint8_t frame6_counter = 0xB;
  uint8_t frame7_counter = 0x5;
  uint8_t BMS_SOH = 99;
  uint8_t BMS_min_cell_voltage_number = 0;
  uint8_t BMS_min_temp_module_number = 0;
  uint8_t BMS_max_cell_voltage_number = 0;
  uint8_t BMS_max_temp_module_number = 0;
  uint8_t battery_frame_index = 0;
  uint8_t discharge_status = 14;
  uint8_t increaseTimeoutSOC = 0;
  static const uint8_t REJECTED = 1;
  static const uint8_t APPROVED = 2;
  uint8_t servicemode = NOT_DETERMINED_YET;
  uint8_t secondsSinceStartup = 0;

  bool BMS_voltage_available = false;

  int16_t battery_daughterboard_temperatures[13] = {-40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40, -40};
  uint16_t battery_cellvoltages[MAX_AMOUNT_CELLS] = {0};

  /* Extra CAN info 
  - 0x40D supposedly has vehicle model in frame1
  - 0x2B6 contains date in frame0-6
  - 
  
    */

  /*12D 
  - Byte0(bits7:6) = IG1 Relay state
  - Byte1(bits3:2) = IG3 Relay state
  - Byte1(bits5:4) = IG4 Relay state*/
  CAN_frame ATTO_3_12D = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x12D,
                          .data = {0xA0, 0x28, 0x02, 0xA0, 0x0C, 0x71, 0xCF, 0x49}};
  CAN_frame ATTO_3_441 = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x441,
                          .data = {0x98, 0x3A, 0x88, 0x13, 0x07, 0x00, 0xFF, 0x8C}};
  CAN_frame ATTO_3_7E7_POLL = {.FD = false,
                               .ext_ID = false,
                               .DLC = 8,
                               .ID = 0x7E7,  //Poll PID 03 22 00 05 (POLL_FOR_BATTERY_SOC)
                               .data = {0x03, 0x22, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ATTO_3_7E7_ACK = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x7E7,  //ACK frame for long PIDs
                              .data = {0x30, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ATTO_3_7E7_CLEAR_CRASH = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 8,
                                      .ID = 0x7E7,
                                      .data = {0x02, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ATTO_3_7E7_RESET_SOC = {.FD = false,
                                    .ext_ID = false,
                                    .DLC = 8,
                                    .ID = 0x7E7,  //This sets SOC to 100.00% (0x27 10) , and AH to 150.00 (0x3A 98)
                                    .data = {0x07, 0x2E, 0x1F, 0xFC, 0x10, 0x27, 0x98, 0x3A}};
};

#endif
