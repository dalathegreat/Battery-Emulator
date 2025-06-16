#ifndef INVERTER_PROTOCOL_H
#define INVERTER_PROTOCOL_H

enum class InverterProtocolType { None = 0, BydCan, BydModbus, FerroampCan, Highest };

extern InverterProtocolType user_selected_inverter_protocol;

extern std::vector<InverterProtocolType> supported_inverter_protocols();
extern const char* name_for_type(InverterProtocolType type);

enum class InverterInterfaceType { Can, Rs485, Modbus };

// The abstract base class for all inverter protocols
class InverterProtocol {
 public:
  virtual void setup() = 0;
  virtual const char* interface_name() = 0;
  virtual InverterInterfaceType interface_type() = 0;

  // This function maps all the values fetched from battery to the correct battery emulator data structures
  virtual void update_values() = 0;
};

extern InverterProtocol* inverter;

#endif
