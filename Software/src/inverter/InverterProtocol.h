#ifndef INVERTER_PROTOCOL_H
#define INVERTER_PROTOCOL_H

enum class InverterProtocolType {
  None = 0,
  AforeCan,
  BydCan,
  BydModbus,
  FerroampCan,
  Foxess,
  GrowattHv,
  GrowattLv,
  Kostal,
  Pylon,
  PylonLv,
  Schneider,
  SmaBydH,
  SmaBydHvs,
  SmaLv,
  SmaTripower,
  Sofar,
  Solax,
  Sungrow,
  Highest
};

extern InverterProtocolType user_selected_inverter_protocol;

extern std::vector<InverterProtocolType> supported_inverter_protocols();
extern const char* name_for_inverter_type(InverterProtocolType type);

enum class InverterInterfaceType { Can, Rs485, Modbus };

// The abstract base class for all inverter protocols
class InverterProtocol {
 public:
  virtual void setup() = 0;
  virtual const char* interface_name() = 0;
  virtual InverterInterfaceType interface_type() = 0;

  // This function maps all the values fetched from battery to the correct battery emulator data structures
  virtual void update_values() = 0;

  // If true, this inverter supports a signal to control contactor (allows_contactor_closing)
  virtual bool controls_contactor() { return false; }

  virtual bool allows_contactor_closing() { return false; }
};

extern InverterProtocol* inverter;

#endif
