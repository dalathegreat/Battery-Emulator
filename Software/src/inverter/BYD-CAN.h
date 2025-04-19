#ifndef BYD_CAN_H
#define BYD_CAN_H
#include "../include.h"
#include "Inverter.h"

#define CAN_INVERTER_SELECTED
#define FW_MAJOR_VERSION 0x03
#define FW_MINOR_VERSION 0x29

void send_intial_data();

class BydCanInverter : public InverterProtocol {
 public:
  virtual void setup();
  virtual void transmit_can();
  virtual void update_values_can_inverter();
  virtual void map_can_frame_to_variable_inverter(CAN_frame rx_frame);

  virtual const char* name() { return Name; }
  static constexpr char* Name = "BYD Battery-Box Premium HVS over CAN Bus";
};

#endif
