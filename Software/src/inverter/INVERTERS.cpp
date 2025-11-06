#include "INVERTERS.h"

InverterProtocol* inverter = nullptr;

InverterProtocolType user_selected_inverter_protocol = InverterProtocolType::BydModbus;

// Some user-configurable settings that can be used by inverters. These
// inverters should use sensible defaults if the corresponding user_selected
// value is zero.
uint16_t user_selected_pylon_send = 0;
uint16_t user_selected_inverter_cells = 0;
uint16_t user_selected_inverter_modules = 0;
uint16_t user_selected_inverter_cells_per_module = 0;
uint16_t user_selected_inverter_voltage_level = 0;
uint16_t user_selected_inverter_ah_capacity = 0;
uint16_t user_selected_inverter_battery_type = 0;
bool user_selected_inverter_ignore_contactors = false;
bool user_selected_pylon_30koffset = false;
bool user_selected_pylon_invert_byteorder = false;
bool user_selected_inverter_deye_workaround = false;

std::vector<InverterProtocolType> supported_inverter_protocols() {
  std::vector<InverterProtocolType> types;

  for (int i = 0; i < (int)InverterProtocolType::Highest; i++) {
    types.push_back((InverterProtocolType)i);
  }

  return types;
}

extern const char* name_for_inverter_type(InverterProtocolType type) {
  switch (type) {
    case InverterProtocolType::None:
      return "None";

    case InverterProtocolType::AforeCan:
      return AforeCanInverter::Name;

    case InverterProtocolType::BydCan:
      return BydCanInverter::Name;

    case InverterProtocolType::BydModbus:
      return BydModbusInverter::Name;

    case InverterProtocolType::FerroampCan:
      return FerroampCanInverter::Name;

    case InverterProtocolType::Foxess:
      return FoxessCanInverter::Name;

    case InverterProtocolType::GrowattHv:
      return GrowattHvInverter::Name;

    case InverterProtocolType::GrowattLv:
      return GrowattLvInverter::Name;

    case InverterProtocolType::GrowattWit:
      return GrowattWitInverter::Name;

    case InverterProtocolType::Kostal:
      return KostalInverterProtocol::Name;

    case InverterProtocolType::Pylon:
      return PylonInverter::Name;

    case InverterProtocolType::PylonLv:
      return PylonLvInverter::Name;

    case InverterProtocolType::Schneider:
      return SchneiderInverter::Name;

    case InverterProtocolType::SmaBydH:
      return SmaBydHInverter::Name;

    case InverterProtocolType::SmaBydHvs:
      return SmaBydHvsInverter::Name;

    case InverterProtocolType::SmaLv:
      return SmaLvInverter::Name;

    case InverterProtocolType::SmaTripower:
      return SmaTripowerInverter::Name;

    case InverterProtocolType::Sofar:
      return SofarInverter::Name;

    case InverterProtocolType::Solax:
      return SolaxInverter::Name;

    case InverterProtocolType::Solxpow:
      return SolxpowInverter::Name;

    case InverterProtocolType::SolArkLv:
      return SolArkLvInverter::Name;

    case InverterProtocolType::Sungrow:
      return SungrowInverter::Name;

    case InverterProtocolType::Highest:
      return "None";
  }
  return nullptr;
}

bool setup_inverter() {
  if (inverter) {
    return true;
  }

  switch (user_selected_inverter_protocol) {
    case InverterProtocolType::AforeCan:
      inverter = new AforeCanInverter();
      break;

    case InverterProtocolType::BydCan:
      inverter = new BydCanInverter();
      break;

    case InverterProtocolType::BydModbus:
      inverter = new BydModbusInverter();
      break;

    case InverterProtocolType::FerroampCan:
      inverter = new FerroampCanInverter();
      break;

    case InverterProtocolType::Foxess:
      inverter = new FoxessCanInverter();
      break;

    case InverterProtocolType::GrowattHv:
      inverter = new GrowattHvInverter();
      break;

    case InverterProtocolType::GrowattLv:
      inverter = new GrowattLvInverter();
      break;

    case InverterProtocolType::GrowattWit:
      inverter = new GrowattWitInverter();
      break;

    case InverterProtocolType::Kostal:
      inverter = new KostalInverterProtocol();
      break;

    case InverterProtocolType::Pylon:
      inverter = new PylonInverter();
      break;

    case InverterProtocolType::PylonLv:
      inverter = new PylonLvInverter();
      break;

    case InverterProtocolType::Schneider:
      inverter = new SchneiderInverter();
      break;

    case InverterProtocolType::SmaBydH:
      inverter = new SmaBydHInverter();
      break;

    case InverterProtocolType::SmaBydHvs:
      inverter = new SmaBydHvsInverter();
      break;

    case InverterProtocolType::SmaLv:
      inverter = new SmaLvInverter();
      break;

    case InverterProtocolType::SmaTripower:
      inverter = new SmaTripowerInverter();
      break;

    case InverterProtocolType::Sofar:
      inverter = new SofarInverter();
      break;

    case InverterProtocolType::Solax:
      inverter = new SolaxInverter();
      break;

    case InverterProtocolType::Solxpow:
      inverter = new SolxpowInverter();
      break;

    case InverterProtocolType::SolArkLv:
      inverter = new SolArkLvInverter();
      break;

    case InverterProtocolType::Sungrow:
      inverter = new SungrowInverter();
      break;

    case InverterProtocolType::None:
      return true;
    case InverterProtocolType::Highest:
    default:
      inverter = nullptr;  // Or handle as error
      break;
  }

  if (inverter) {
    return inverter->setup();
  }

  return false;
}
