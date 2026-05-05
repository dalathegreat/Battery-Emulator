#ifndef MICROVAST_BATTERY_H
#define MICROVAST_BATTERY_H

#include "CanBattery.h"

class MicrovastBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Microvast Battery";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 5000;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 2500;
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4200;
  static const int MIN_CELL_VOLTAGE_MV = 2700;

  unsigned long previousMillis20 = 0;    // will store last time a 20ms CAN Message was send
  unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was send
  uint8_t counter_1s = 15;               // counter for the 1s message, cycles between 0-1-2-3..15-0-1...

  uint16_t battery_voltage = 3700;
  uint16_t system_voltage = 0;
  uint16_t SOH_ppt = 990;
  uint16_t SOC_ppt = 5000;
  uint16_t cellvoltage_min_mv = 3700;
  uint16_t cellvoltage_max_mv = 3700;
  int16_t lowest_cell_temperature_C = 0;
  int16_t highest_cell_temperature_C = 0;
  int16_t battery_current = 0;
  int16_t discharge_current_max = 0;
  int16_t charge_current_max = 0;

  CAN_frame MICRO_0C20FFF4 = {.FD = false,
                              .ext_ID = true,
                              .DLC = 8,
                              .ID = 0x0C20FFF4,
                              .data = {0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame MICRO_cee00a0 = {.FD = false,
                             .ext_ID = true,
                             .DLC = 7,
                             .ID = 0xcee00a0,  //frame0 CRC, frame1 counter, frame2 interlock OK
                             .data = {0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00}};
#  CAN_frame MICRO_084 = {.FD = false,
#                         .ext_ID = false,
#                         .DLC = 8,
#                         .ID = 0x084,
#                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
#};

#endif
