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

 private:
  static const int MAX_PACK_VOLTAGE_DV = 5000;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 1500;  //TODO SET
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  unsigned long previousMillis20 = 0;  // will store last time a 20ms CAN Message was send

  uint8_t expectedCRC = 0;
};

#endif
