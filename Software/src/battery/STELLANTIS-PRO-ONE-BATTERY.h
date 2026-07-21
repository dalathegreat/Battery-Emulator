#ifndef STELLANTIS_PRO_ONE_BATTERY_H
#define STELLANTIS_PRO_ONE_BATTERY_H
#include "../datalayer/datalayer.h"
#include "CanBattery.h"

class StellantisProOneBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Stellantis Pro One 110kWh (E-Ducato/ProMaster/Proace)";

  bool supports_reset_DTC() { return true; }
  void reset_DTC() { UserRequestedDTCReset = true; }

 private:
  bool UserRequestedDTCReset = false;
  static const int MAX_PACK_VOLTAGE_DV = 5000;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 1500;  //TODO SET
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  static constexpr CAN_frame STELLANTIS_CLEAR_DTC = {.FD = false,
                                                     .ext_ID = false,
                                                     .DLC = 5,
                                                     .ID = 0x7E7,
                                                     .data = {0x04, 0x14, 0xFF, 0xFF, 0xFF}};
  CAN_frame ONE_15A = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x15A, .data = {0x00, 0x00, 0x00, 0x00}};
  CAN_frame ONE_1D7 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x1D7,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ONE_175 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x175,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ONE_108 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x108,
                       .data = {0x00, 0x00, 0x00, 0x3D, 0x09, 0x00, 0x00, 0x00}};

  unsigned long previousMillis10 = 0;    // will store last time a 10ms CAN Message was sent
  unsigned long previousMillis20 = 0;    // will store last time a 20ms CAN Message was sent
  unsigned long previousMillis50 = 0;    // will store last time a 50ms CAN Message was sent
  unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was sent
  unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was sent
  uint8_t expectedCRC = 0;
  uint8_t counter_10ms = 0;        //Counter for the 10ms CAN message, goes from 0-0xF and starts over
  uint8_t sent_10ms_messages = 0;  //Counter for the number of 10ms messages sent, goes from 0-0xFF and starts over
};

#endif
