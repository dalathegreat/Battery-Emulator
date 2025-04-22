#ifndef ATTO_3_BATTERY_H
#define ATTO_3_BATTERY_H

#include "../datalayer/datalayer.h"
#include "../include.h"

#define USE_ESTIMATED_SOC  // If enabled, SOC is estimated from pack voltage. Useful for locked packs. \
                           // Comment out this only if you know your BMS is unlocked and able to send SOC%
#define MAXPOWER_CHARGE_W 10000
#define MAXPOWER_DISCHARGE_W 10000

//Uncomment and configure this line, if you want to filter out a broken temperature sensor (1-10)
//Make sure you understand risks associated with disabling. Values can be read via "More Battery info"
//#define SKIP_TEMPERATURE_SENSOR_NUMBER 1

/* Do not modify the rows below */
#define BATTERY_SELECTED
#define NOT_DETERMINED_YET 0
#define STANDARD_RANGE 1
#define EXTENDED_RANGE 2

class BydAtto3Battery : public CanBattery {
 public:
  BydAtto3Battery(DATALAYER_BATTERY_TYPE* target, CAN_Interface can_interface) : CanBattery(BydAtto3) {
    m_target = target;
    m_can_interface = can_interface;
  }
  virtual const char* name() { return Name; };
  static constexpr const char* Name = "BYD Atto 3";

  virtual void setup();
  virtual void update_values();
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void transmit_can();

  virtual uint16_t max_pack_voltage_dv() { 
    if (battery_type == STANDARD_RANGE) {
      return 3640;
    } else if (battery_type == EXTENDED_RANGE) {
      return 4410;
    } else {
      return 0;
    }
  };
  virtual uint16_t min_pack_voltage_dv() { 
    if (battery_type == STANDARD_RANGE) {
      return 3136;
    } else if (battery_type == EXTENDED_RANGE) {
      return 3800;
    } else {
      return 0;
    }
  };
  virtual uint16_t max_cell_deviation_mv() { return 150; }
  virtual uint16_t max_cell_voltage_mv() { return 3800; }
  virtual uint16_t min_cell_voltage_mv() { return 2800; }
  virtual uint8_t number_of_cells() { 
    if (battery_type == STANDARD_RANGE) {
      return 104;
    } else if (battery_type == EXTENDED_RANGE) {
      return 126;
    } else {
      return 0;
    }
  }

 private:
  DATALAYER_BATTERY_TYPE* m_target;
  CAN_Interface m_can_interface;

  uint8_t battery_type = NOT_DETERMINED_YET;
  unsigned long previousMillis50 = 0;   // will store last time a 50ms CAN Message was send
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis500 = 0;  // will store last time a 500ms CAN Message was send
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
  uint16_t BMS_SOC = 0;
  uint16_t BMS_voltage = 0;
  int16_t BMS_current = 0;
  int16_t BMS_lowest_cell_temperature = 0;
  int16_t BMS_highest_cell_temperature = 0;
  int16_t BMS_average_cell_temperature = 0;
  uint16_t BMS_lowest_cell_voltage_mV = 3300;
  uint16_t BMS_highest_cell_voltage_mV = 3300;
  uint8_t battery_frame_index = 0;
#define NOF_CELLS 126
  uint16_t battery_cellvoltages[NOF_CELLS] = {0};
#define POLL_FOR_BATTERY_SOC 0x05
#define POLL_FOR_BATTERY_VOLTAGE 0x08
#define POLL_FOR_BATTERY_CURRENT 0x09
#define POLL_FOR_LOWEST_TEMP_CELL 0x2f
#define POLL_FOR_HIGHEST_TEMP_CELL 0x31
#define POLL_FOR_BATTERY_PACK_AVG_TEMP 0x32
#define POLL_FOR_BATTERY_CELL_MV_MAX 0x2D
#define POLL_FOR_BATTERY_CELL_MV_MIN 0x2B
#define UNKNOWN_POLL_1 0xFC
#define ESTIMATED 0
#define MEASURED 1
  uint16_t poll_state = POLL_FOR_BATTERY_SOC;

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
};

#endif
