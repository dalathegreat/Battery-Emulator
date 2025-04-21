#ifndef SCHNEIDER_CAN_H
#define SCHNEIDER_CAN_H
#include "../include.h"

#define CAN_INVERTER_SELECTED

#define STATE_OFFLINE 0
#define STATE_STANDBY 1
#define STATE_STARTING 2
#define STATE_ONLINE 3
#define STATE_FAULTED 4

// Same enumerations used for Fault and Warning
#define FAULTS_CHARGE_OVERCURRENT 0
#define FAULTS_DISCHARGE_OVERCURRENT 1
#define FAULTS_OVER_TEMPERATURE 2
#define FAULTS_UNDER_TEMPERATURE 3
#define FAULTS_OVER_VOLTAGE 4
#define FAULTS_UNDER_VOLTAGE 5
#define FAULTS_CELL_IMBALANCE 6
#define FAULTS_INTERNAL_COM_ERROR 7
#define FAULTS_SYSTEM_ERROR 8

// Commands. Bit0 forced charge request. Bit1 charge permitted. Bit2 discharge permitted. Bit3 Stop
#define COMMAND_ONLY_CHARGE_ALLOWED 0x02
#define COMMAND_ONLY_DISCHARGE_ALLOWED 0x04
#define COMMAND_CHARGE_AND_DISCHARGE_ALLOWED 0x06
#define COMMAND_STOP 0x08

class SchneiderCanInverter : public InverterProtocol {
 public:
  virtual void transmit_can();
  virtual void update_values_can_inverter();
  virtual void map_can_frame_to_variable_inverter(CAN_frame rx_frame);

  virtual const char* name() { return Name; };
  static constexpr char* Name = "Schneider V2 SE BMS CAN";
};

#endif
