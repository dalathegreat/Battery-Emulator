#ifndef SMA_LV_CAN_H
#define SMA_LV_CAN_H
#include "../include.h"
#include "Inverter.h"

#define CAN_INVERTER_SELECTED

#define READY_STATE 0x03
#define STOP_STATE 0x02

class SmaLvCanInverter : public InverterProtocol {
 public:
  virtual void transmit_can();
  virtual void update_values_can_inverter();

  virtual const char* name() { return Name; };
  static constexpr char* Name = "SMA Low Voltage (48V) protocol via CAN";
};

#endif
