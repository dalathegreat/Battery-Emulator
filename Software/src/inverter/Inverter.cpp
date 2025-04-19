#include "Inverter.h"
#include "INVERTERS.h"

InverterProtocolType userSelectedInverter = InverterProtocolType::BydModbus;

InverterProtocol* init_inverter_protocol(InverterProtocolType type) {
  switch (type) {
#ifdef BYD_MODBUS
    case BydModbus:
      inverter = new BydModbusInverter();
      break;
#endif

#ifdef AFORE_CAN
    case AforeCan:
      inverter = new AforeCanInverter();
      break;
#endif

#ifdef FERROAMP_CAN
    case FerroampCan:
      inverter = new FerroampCanInverter();
      break;
#endif

#ifdef FOXESS_CAN
    case FoxessCan:
      inverter = new FoxessCanInverter();
      break;
#endif

#ifdef GROWATT_HV_CAN
    case GrowattHvCan:
      inverter = new GrowattHvInverter();
      break;
#endif

#ifdef GROWATT_LV_CAN
    case GrowattLvCan:
      inverter = new GrowattLvCanInverter();
      break;
#endif

#ifdef BYD_KOSTAL_RS485
    case KostalRs485:
      inverter = new KostalRs485Inverter();
      break;
#endif

      /*#ifdef PYLON_CAN
        case PylonCan:
            inverter = new PylonLvCanInverter();
            break;
    #endif*/

#ifdef PYLON_LV_CAN
    case PylonLvCan:
      inverter = new PylonLvCanInverter();
      break;
#endif

#ifdef SCHNEIDER_CAN
    case SchneiderCan:
      inverter = new SchneiderCanInverter();
      break;
#endif

#ifdef SMA_BYD_H_CAN
    case SmaBydHCan:
      inverter = new SmaBydHCanInverter();
      break;
#endif

#ifdef SMA_BYD_HVS_CAN
    case SmaBydHsvCan:
      inverter = new SmaBydHvsCanInverter();
      break;
#endif

#ifdef SMA_LV_CAN
    case SmaLvCan:
      inverter = new SmaLvCanInverter();
      break;
#endif

#ifdef SMA_TRIPOWER_CAN
    case SmaTripowerCan:
      inverter = new SmaTripowerCanInverter();
      break;
#endif

#ifdef SOLAX_CAN
    case Solax:
      inverter = new SolaxCanInverter();
      break;
#endif

#ifdef SOFAR_CAN
    case Sofar:
      inverter = new SofarCanInverter();
      break;
#endif

#ifdef SUNGROW_CAN
    case Sungrow:
      inverter = new SungrowCanInverter();
      break;
#endif

    default:
      return nullptr;
  }

  return inverter;
}

const char* InverterProtocol::name_for_type(InverterProtocolType type) {
  switch (type) {
#ifdef BYD_MODBUS
    case BydModbus:
      return BydModbusInverter::Name;
#endif

#ifdef AFORE_CAN
    case AforeCan:
      return AforeCanInverter::Name;
#endif

#ifdef FERROAMP_CAN
    case FerroampCan:
      return FerroampCanInverter::Name;
#endif

#ifdef FOXESS_CAN
    case FoxessCan:
      return FoxessCanInverter::Name;
#endif

#ifdef GROWATT_HV_CAN
    case GrowattHvCan:
      return GrowattHvInverter::Name;
#endif

#ifdef GROWATT_LV_CAN
    case GrowattLvCan:
      return GrowattLvCanInverter::Name;
#endif

#ifdef BYD_KOSTAL_RS485
    case KostalRs485:
      return KostalRs485Inverter::Name;
#endif

#ifdef PYLON_LV_CAN
    case PylonLvCan:
      return PylonLvCanInverter::Name;
#endif

#ifdef SCHNEIDER_CAN
    case SchneiderCan:
      return SchneiderCanInverter::Name;
#endif

#ifdef SMA_BYD_H_CAN
    case SmaBydHCan:
      return SmaBydHCanInverter::Name;
#endif

#ifdef SMA_BYD_HVS_CAN
    case SmaBydHsvCan:
      return SmaBydHvsCanInverter::Name;
#endif

#ifdef SMA_LV_CAN
    case SmaLvCan:
      return SmaLvCanInverter::Name;
#endif

#ifdef SMA_TRIPOWER_CAN
    case SmaTripowerCan:
      return SmaTripowerCanInverter::Name;
#endif

#ifdef SOLAX_CAN
    case Solax:
      return SolaxCanInverter::Name;
#endif

#ifdef SOFAR_CAN
    case Sofar:
      return SofarCanInverter::Name;
#endif

#ifdef SUNGROW_CAN
    case Sungrow:
      return SungrowCanInverter::Name;
#endif

    default:
      return nullptr;
  }
}

std::vector<InverterProtocolType> supported_inverter_protocols() {
  std::vector<InverterProtocolType> types;

  for (int i = 1; i < static_cast<int>(HighestInverter); i++) {
    auto type = static_cast<InverterProtocolType>(i);
    if (InverterProtocol::name_for_type(type) != nullptr) {
      types.push_back(type);
    }
  }

  return types;
}
