#ifndef INVERTER_PROTOCOL_H
#define INVERTER_PROTOCOL_H

#include <vector>

enum class InverterProtocolType {
  None = 0,
  AforeCan = 1,
  BydCan = 2,
  BydModbus = 3,
  FerroampCan = 4,
  Foxess = 5,
  GrowattHv = 6,
  GrowattLv = 7,
  GrowattWit = 8,
  Kostal = 9,
  Pylon = 10,
  PylonLv = 11,
  Schneider = 12,
  SmaBydH = 13,
  SmaBydHvs = 14,
  SmaLv = 15,
  SmaTripower = 16,
  Sofar = 17,
  Solax = 18,
  Solxpow = 19,
  SolArkLv = 20,
  Sungrow = 21,
  Highest
};

extern InverterProtocolType user_selected_inverter_protocol;

extern std::vector<InverterProtocolType> supported_inverter_protocols();
extern const char* name_for_inverter_type(InverterProtocolType type);

enum class InverterInterfaceType { Can, Rs485, Modbus };

// The abstract base class for all inverter protocols
class InverterProtocol {
 public:
  virtual const char* name() = 0;
  virtual bool setup() { return true; }
  virtual const char* interface_name() = 0;
  virtual InverterInterfaceType interface_type() = 0;

  // This function maps all the values fetched from battery to the correct battery emulator data structures
  virtual void update_values() = 0;

  // If true, this inverter supports a signal to control contactor (allows_contactor_closing)
  virtual bool controls_contactor() { return false; }

  virtual bool allows_contactor_closing() { return false; }

  virtual bool supports_battery_id() { return false; }
};

extern InverterProtocol* inverter;

#endif
