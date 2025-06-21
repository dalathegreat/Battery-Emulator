#ifndef KIA_HYUNDAI_HYBRID_BATTERY_H
#define KIA_HYUNDAI_HYBRID_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#include "CanBattery.h"

#ifdef KIA_HYUNDAI_HYBRID_BATTERY
#define SELECTED_BATTERY_CLASS KiaHyundaiHybridBattery
#endif

class KiaHyundaiHybridBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr char* Name = "Kia/Hyundai Hybrid";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 2550;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 1700;
  static const int MAX_CELL_DEVIATION_MV = 100;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  unsigned long previousMillis1000 = 0;  // will store last time a 100ms CAN Message was send

  uint16_t SOC = 0;
  uint16_t SOC_display = 0;
  bool interlock_missing = false;
  int16_t battery_current = 0;
  uint8_t battery_current_high_byte = 0;
  uint16_t battery_voltage = 0;
  uint32_t available_charge_power = 0;
  uint32_t available_discharge_power = 0;
  int8_t battery_module_max_temperature = 0;
  int8_t battery_module_min_temperature = 0;
  uint8_t poll_data_pid = 0;
  uint16_t cellvoltages_mv[98];
  uint16_t min_cell_voltage_mv = 3700;
  uint16_t max_cell_voltage_mv = 3700;

  CAN_frame KIA_7E4_id1 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x7E4,
                           .data = {0x02, 0x21, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame KIA_7E4_id2 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x7E4,
                           .data = {0x02, 0x21, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame KIA_7E4_id3 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x7E4,
                           .data = {0x02, 0x21, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame KIA_7E4_id5 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x7E4,
                           .data = {0x02, 0x21, 0x05, 0x04, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame KIA_7E4_ack = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x7E4,  //Ack frame, correct PID is returned. Flow control message
                           .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
};

#endif
