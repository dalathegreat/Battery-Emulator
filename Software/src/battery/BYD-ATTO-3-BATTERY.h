#ifndef ATTO_3_BATTERY_H
#define ATTO_3_BATTERY_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../include.h"

#include "BYD-ATTO-3-HTML.h"
#include "CanBattery.h"

#define USE_ESTIMATED_SOC  // If enabled, SOC is estimated from pack voltage. Useful for locked packs. \
                           // Comment out this only if you know your BMS is unlocked and able to send SOC%
#define MAXPOWER_CHARGE_W 10000
#define MAXPOWER_DISCHARGE_W 10000

//Uncomment and configure this line, if you want to filter out a broken temperature sensor (1-10)
//Make sure you understand risks associated with disabling. Values can be read via "More Battery info"
//#define SKIP_TEMPERATURE_SENSOR_NUMBER 1

/* Do not modify the rows below */
#ifdef BYD_ATTO_3_BATTERY
#define SELECTED_BATTERY_CLASS BydAttoBattery
#endif

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

  static constexpr char* Name = "BYD Atto 3";

  bool supports_charged_energy() { return true; }
  bool supports_reset_crash() { return true; }

  void reset_crash() { datalayer_bydatto->UserRequestCrashReset = true; }

#ifndef USE_ESTIMATED_SOC
  // Toggle SOC method in UI is only enabled if we initially use measured SOC
  bool supports_toggle_SOC_method() { return true; }
#endif

  void toggle_SOC_method() { SOC_method = !SOC_method; }

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  static const int CELLCOUNT_EXTENDED = 126;
  static const int CELLCOUNT_STANDARD = 104;
  static const int MAX_PACK_VOLTAGE_EXTENDED_DV = 4410;  //Extended range
  static const int MIN_PACK_VOLTAGE_EXTENDED_DV = 3800;  //Extended range
  static const int MAX_PACK_VOLTAGE_STANDARD_DV = 3640;  //Standard range
  static const int MIN_PACK_VOLTAGE_STANDARD_DV = 3136;  //Standard range
  static const int MAX_CELL_DEVIATION_MV = 230;
  static const int MAX_CELL_VOLTAGE_MV = 3650;  //Charging stops if one cell exceeds this value
  static const int MIN_CELL_VOLTAGE_MV = 2800;  //Discharging stops if one cell goes below this value

  BydAtto3HtmlRenderer renderer;
  DATALAYER_BATTERY_TYPE* datalayer_battery;
  DATALAYER_INFO_BYDATTO3* datalayer_bydatto;
  bool* allows_contactor_closing;

  static const int POLL_FOR_BATTERY_SOC = 0x0005;
  static const uint8_t NOT_DETERMINED_YET = 0;
  static const uint8_t STANDARD_RANGE = 1;
  static const uint8_t EXTENDED_RANGE = 2;

  static const uint8_t NOT_RUNNING = 0xFF;
  static const uint8_t STARTED = 0;
  static const uint8_t RUNNING_STEP_1 = 1;
  static const uint8_t RUNNING_STEP_2 = 2;

  uint8_t battery_type = NOT_DETERMINED_YET;
  uint8_t stateMachineClearCrash = NOT_RUNNING;
  unsigned long previousMillis50 = 0;   // will store last time a 50ms CAN Message was send
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis200 = 0;  // will store last time a 200ms CAN Message was send
  bool SOC_method = false;
  uint8_t counter_50ms = 0;
  uint8_t counter_100ms = 0;
  uint8_t frame6_counter = 0xB;
  uint8_t frame7_counter = 0x5;
  uint16_t battery_voltage = 0;
  int16_t battery_temperature_ambient = 0;
  int16_t battery_daughterboard_temperatures[10];
  int16_t battery_lowest_temperature = 0;
  int16_t battery_highest_temperature = 0;
  int16_t battery_calc_min_temperature = 0;
  int16_t battery_calc_max_temperature = 0;
  uint16_t battery_highprecision_SOC = 0;
  uint16_t battery_estimated_SOC = 0;
  uint16_t BMS_SOC = 0;
  uint16_t BMS_voltage = 0;
  int16_t BMS_current = 0;
  int16_t BMS_lowest_cell_temperature = 0;
  int16_t BMS_highest_cell_temperature = 0;
  int16_t BMS_average_cell_temperature = 0;
  uint16_t BMS_lowest_cell_voltage_mV = 3300;
  uint16_t BMS_highest_cell_voltage_mV = 3300;
  uint32_t BMS_unknown0 = 0;
  uint32_t BMS_unknown1 = 0;
  uint16_t BMS_allowed_charge_power = 0;
  uint16_t BMS_unknown3 = 0;
  uint16_t BMS_unknown4 = 0;
  uint16_t BMS_total_charged_ah = 0;
  uint16_t BMS_total_discharged_ah = 0;
  uint16_t BMS_total_charged_kwh = 0;
  uint16_t BMS_total_discharged_kwh = 0;
  uint16_t BMS_unknown9 = 0;
  uint8_t BMS_unknown10 = 0;
  uint8_t BMS_unknown11 = 0;
  uint8_t BMS_unknown12 = 0;
  uint8_t BMS_unknown13 = 0;
  uint8_t battery_frame_index = 0;
  uint16_t battery_cellvoltages[CELLCOUNT_EXTENDED] = {0};

  uint16_t poll_state = POLL_FOR_BATTERY_SOC;
  uint16_t pid_reply = 0;

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
};

#endif
