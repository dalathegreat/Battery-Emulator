#ifndef SOLAX_CAN_H
#define SOLAX_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

// Timeout in milliseconds
#define SolaxTimeout 2000

//SOLAX BMS States Definition
#define BATTERY_ANNOUNCE 0
#define WAITING_FOR_CONTACTOR 1
#define CONTACTOR_CLOSED 2
#define FAULT_SOLAX 3
#define UPDATING_FW 4

class SolaxCanInverter : public InverterProtocol {
 public:
  virtual void setup();
  virtual void transmit_can();
  virtual void update_values_can_inverter();
  virtual void map_can_frame_to_variable_inverter(CAN_frame rx_frame);

  virtual const char* name() { return Name; };
  static constexpr char* Name = "SolaX Triple Power LFP over CAN bus";
};

#endif
