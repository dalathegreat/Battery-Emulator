#include "../include.h"

InverterProtocol* inverter = nullptr;

InverterProtocolType user_selected_inverter_protocol = InverterProtocolType::BydModbus;

std::vector<InverterProtocolType> supported_inverter_protocols() {
  std::vector<InverterProtocolType> types;

  for (int i = 0; i < (int)InverterProtocolType::Highest; i++) {
    types.push_back((InverterProtocolType)i);
  }

  return types;
}

extern const char* name_for_type(InverterProtocolType type) {
  switch (type) {
    case InverterProtocolType::BydCan:
      return BydCanInverter::Name;
      break;
    case InverterProtocolType::BydModbus:
      return BydModbusInverter::Name;
      break;
    case InverterProtocolType::FerroampCan:
      return FerroampCanInverter::Name;
      break;
    case InverterProtocolType::None:
      return "None";
      break;
  }

  return nullptr;
}

#ifdef COMMON_IMAGE
#ifdef SELECTED_INVERTER_CLASS
#error "Compile time SELECTED_INVERTER_CLASS should not be defined with COMMON_IMAGE"
#endif

void setup_inverter() {
  if (inverter) {
    return;
  }

  switch (user_selected_inverter_protocol) {
    case InverterProtocolType::BydCan:
      inverter = new BydCanInverter();
      break;
    case InverterProtocolType::BydModbus:
      inverter = new BydModbusInverter();
      break;
    case InverterProtocolType::FerroampCan:
      inverter = new FerroampCanInverter();
      break;
  }

  if (inverter) {
    inverter->setup();
  }
}

#else
void setup_inverter() {
  if (inverter) {
    // The inverter is setup only once.
    return;
  }

#ifdef SELECTED_INVERTER_CLASS
  inverter = new SELECTED_INVERTER_CLASS();

  if (inverter) {
    inverter->setup();
  }
#endif
}
#endif
