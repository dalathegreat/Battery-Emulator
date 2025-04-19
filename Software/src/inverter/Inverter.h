#ifndef INVERTER_H
#define INVERTER_H

#include <vector>
#include "src/devboard/utils/types.h"

// Enum number values used in user settings. Retain numbering.
enum InverterProtocolType {
  AforeCan = 1,
  BydCan = 2,
  BydModbus = 3,
  FerroampCan = 4,
  FoxessCan = 5,
  GrowattHvCan = 6,
  GrowattLvCan = 7,
  KostalRs485 = 8,
  PylonCan = 9,
  PylonLvCan = 10,
  SchneiderCan = 11,
  SmaBydHCan = 12,
  SmaBydHsvCan = 13,
  SmaLvCan = 14,
  SmaTripowerCan = 15,
  Sofar = 16,
  Solax = 17,
  Sungrow = 18,
  HighestInverter = 19
};

class InverterProtocol {
 public:
  virtual void setup() {}
  virtual void transmit_can_frame(CAN_frame* tx_frame, int interface) {}

  virtual bool usesCAN() { return false; }
  virtual void transmit_can() {}
  virtual void update_values_can_inverter() {}
  virtual void map_can_frame_to_variable_inverter(CAN_frame rx_frame) {}

  virtual bool usesRS485() { return false; }
  virtual void receive_RS485() {}
  virtual int rs485_baudrate() { return 0; }
  virtual void update_RS485_registers_inverter() {}

  virtual bool usesMODBUS() { return false; }
  virtual void update_modbus_registers_inverter() {}
  virtual void handle_static_data_modbus() {}

  virtual const char* name() = 0;
  static const char* name_for_type(InverterProtocolType type);
};

class Rs485Inverter : public InverterProtocol {
 public:
  virtual void setup();
};

InverterProtocol* init_inverter_protocol(InverterProtocolType type);

std::vector<InverterProtocolType> supported_inverter_protocols();

extern InverterProtocol* inverter;
extern InverterProtocolType userSelectedInverter;

#endif
