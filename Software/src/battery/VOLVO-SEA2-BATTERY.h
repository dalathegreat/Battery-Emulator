#ifndef VOLVO_SEA2_BATTERY_H
#define VOLVO_SEA2_BATTERY_H

#include "CanBattery.h"

class VolvoSea2Battery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Volvo/Zeekr/Geely SEA2 battery";

 private:
  static const int MAX_PACK_VOLTAGE_LFP_120S_DV = 4380;
  static const int MIN_PACK_VOLTAGE_LFP_120S_DV = 3600;
  static const int MAX_CAPACITY_LFP_WH = 51000;
  static const int MAX_PACK_VOLTAGE_NCM_107S_DV = 4650;
  static const int MIN_PACK_VOLTAGE_NCM_107S_DV = 3210;
  static const int MAX_CAPACITY_NCM_WH = 69000;
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4260;  // Charging is halted if one cell goes above this
  static const int MIN_CELL_VOLTAGE_MV = 2900;  // Charging is halted if one cell goes below this

  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis1s = 0;   // will store last time a 1s CAN Message was send
  unsigned long previousMillis60s = 0;  // will store last time a 60s CAN Message was send

  CAN_frame VOLVO_536 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x536,
                         .data = {0x00, 0x40, 0x40, 0x01, 0x00, 0x00, 0x00, 0x00}};  //Network manage frame
  CAN_frame VOLVO_372 = {
      .FD = false,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x372,
      .data = {0x00, 0xA6, 0x07, 0x14, 0x04, 0x00, 0x80, 0x00}};  //Ambient Temp -->>VERIFY this data content!!!<<--
};

#endif
