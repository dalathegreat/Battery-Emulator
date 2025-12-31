#ifndef THUNDERSTRUCK_BMS_H
#define THUNDERSTRUCK_BMS_H

#include "CanBattery.h"

class ThunderstruckBMS : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Thunderstruck BMS";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 5000;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 2500;
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 3800;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  unsigned long previousMillis500 = 0;  // will store last time a 500ms CAN Message was send

  uint16_t lowest_cell_voltage = 3700;
  uint16_t highest_cell_voltage = 3700;
  uint16_t packvoltage_dV = 3700;
  uint16_t DCLMin = 0;
  uint16_t CCLMin = 0;
  uint16_t DCLMax = 0;
  uint16_t CCLMax = 0;

  int16_t pack_curent_dA = 0;

  uint8_t SOC = 50;

  int8_t highest_cell_temperature = 0;
  int8_t lowest_cell_temperature = 0;

  CAN_frame THUND_14efd0d8 = {.FD = false,
                              .ext_ID = true,
                              .DLC = 8,
                              .ID = 0x14ebd0d8,
                              .data = {0x20, 0xff, 0x0a, 0x08, 0x00, 0x00, 0x00, 0x00}};
};

#endif
