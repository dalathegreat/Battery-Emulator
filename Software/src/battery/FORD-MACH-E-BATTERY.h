#ifndef FORD_MACH_E_BATTERY_H
#define FORD_MACH_E_BATTERY_H
#include "../datalayer/datalayer.h"
#include "CanBattery.h"

class FordMachEBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Ford Mustang Mach-E battery";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 5000;  //TODO SET
  static const int MIN_PACK_VOLTAGE_DV = 2000;  //TODO SET
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4250;
  static const int MIN_CELL_VOLTAGE_MV = 2900;

  unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis10s = 0;  // will store last time a 1s CAN Message was send

  CAN_frame TEST = {.FD = false,
                    .ext_ID = false,
                    .DLC = 8,
                    .ID = 0x123,
                    .data = {0x10, 0x64, 0x00, 0xB0, 0x00, 0x1E, 0x00, 0x8F}};
};

#endif
