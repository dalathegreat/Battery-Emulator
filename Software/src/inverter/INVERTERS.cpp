#include "INVERTERS.h"
#include "../datalayer/datalayer.h"

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
uint16_t user_selected_inverter_sungrow_type = 0;
uint16_t user_selected_inverter_pylon_type = 0;
bool user_selected_inverter_ignore_contactors = false;
bool user_selected_pylon_30koffset = false;
bool user_selected_pylon_invert_byteorder = false;
bool user_selected_inverter_deye_workaround = false;
bool user_selected_primo_gen24 =
    false;  //Used by BYD-Modbus (Fronius Primo Gen24) inverters to determine if we should cap voltage to 450V or not

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

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_AFORE_CAN)
    case InverterProtocolType::AforeCan:
      return AforeCanInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_BYD_CAN)
    case InverterProtocolType::BydCan:
      return BydCanInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_BYD_MODBUS)
    case InverterProtocolType::BydModbus:
      return BydModbusInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_FERROAMP_CAN)
    case InverterProtocolType::FerroampCan:
      return FerroampCanInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_FOXESS_CAN)
    case InverterProtocolType::Foxess:
      return FoxessCanInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_GROWATT_HV_CAN)
    case InverterProtocolType::GrowattHv:
      return GrowattHvInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_GROWATT_LV_CAN)
    case InverterProtocolType::GrowattLv:
      return GrowattLvInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_GROWATT_WIT_CAN)
    case InverterProtocolType::GrowattWit:
      return GrowattWitInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_KOSTAL_RS485)
    case InverterProtocolType::Kostal:
      return KostalInverterProtocol::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_PYLON_CAN)
    case InverterProtocolType::Pylon:
      return PylonInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_PYLON_LV_CAN)
    case InverterProtocolType::PylonLv:
      return PylonLvInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_PYLON_LV_RS485)
    case InverterProtocolType::PylonLV485:
      return PylonLV485InverterProtocol::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SCHNEIDER_CAN)
    case InverterProtocolType::Schneider:
      return SchneiderInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SMA_BYD_H_CAN)
    case InverterProtocolType::SmaBydH:
      return SmaBydHInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SMA_LV_CAN)
    case InverterProtocolType::SmaLv:
      return SmaLvInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SMA_BYD_HVS_CAN)
    case InverterProtocolType::SmaBydHvs:
      return SmaBydHvsInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SOFAR_CAN)
    case InverterProtocolType::Sofar:
      return SofarInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SOLAX_CAN)
    case InverterProtocolType::Solax:
      return SolaxInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SOLXPOW_CAN)
    case InverterProtocolType::Solxpow:
      return SolxpowInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SOLARK_LV_CAN)
    case InverterProtocolType::SolArkLv:
      return SolArkLvInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SUNGROW_CAN)
    case InverterProtocolType::Sungrow:
      return SungrowInverter::Name;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_VCU_CAN)
    case InverterProtocolType::VCU:
      return VCUInverter::Name;
#endif

    case InverterProtocolType::Highest:
      return "None";

    default:
      return nullptr;
  }
}

bool setup_inverter() {
  if (inverter) {
    return true;
  }

  switch (user_selected_inverter_protocol) {

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_AFORE_CAN)
    case InverterProtocolType::AforeCan:
      inverter = new AforeCanInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_BYD_CAN)
    case InverterProtocolType::BydCan:
      inverter = new BydCanInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_BYD_MODBUS)
    case InverterProtocolType::BydModbus:
      inverter = new BydModbusInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_FERROAMP_CAN)
    case InverterProtocolType::FerroampCan:
      inverter = new FerroampCanInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_FOXESS_CAN)
    case InverterProtocolType::Foxess:
      inverter = new FoxessCanInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_GROWATT_HV_CAN)
    case InverterProtocolType::GrowattHv:
      inverter = new GrowattHvInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_GROWATT_LV_CAN)
    case InverterProtocolType::GrowattLv:
      inverter = new GrowattLvInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_GROWATT_WIT_CAN)
    case InverterProtocolType::GrowattWit:
      inverter = new GrowattWitInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_KOSTAL_RS485)
    case InverterProtocolType::Kostal:
      inverter = new KostalInverterProtocol();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_PYLON_CAN)
    case InverterProtocolType::Pylon:
      inverter = new PylonInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_PYLON_LV_CAN)
    case InverterProtocolType::PylonLv:
      inverter = new PylonLvInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_PYLON_LV_RS485)
    case InverterProtocolType::PylonLV485:
      inverter = new PylonLV485InverterProtocol();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SCHNEIDER_CAN)
    case InverterProtocolType::Schneider:
      inverter = new SchneiderInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SMA_BYD_H_CAN)
    case InverterProtocolType::SmaBydH:
      inverter = new SmaBydHInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SMA_LV_CAN)
    case InverterProtocolType::SmaLv:
      inverter = new SmaLvInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SMA_BYD_HVS_CAN)
    case InverterProtocolType::SmaBydHvs:
      inverter = new SmaBydHvsInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SOFAR_CAN)
    case InverterProtocolType::Sofar:
      inverter = new SofarInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SOLAX_CAN)
    case InverterProtocolType::Solax:
      inverter = new SolaxInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SOLXPOW_CAN)
    case InverterProtocolType::Solxpow:
      inverter = new SolxpowInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SOLARK_LV_CAN)
    case InverterProtocolType::SolArkLv:
      inverter = new SolArkLvInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SUNGROW_CAN)
    case InverterProtocolType::Sungrow:
      inverter = new SungrowInverter();
      break;
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_VCU_CAN)
    case InverterProtocolType::VCU:
      inverter = new VCUInverter();
      break;
#endif

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