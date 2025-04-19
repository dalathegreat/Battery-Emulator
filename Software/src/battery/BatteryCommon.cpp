#include "../include.h"
#include "Battery.h"

BatteryType userSelectedBatteryType = BatteryType::TestFake;

BatteryBase* init_battery(BatteryType type) {

  // To support selecting a battery at compile time, we need to use the
  // #ifdefs here. If this support is not needed, the #ifdefs can be removed.

  switch (type) {
#ifdef BMW_I3_BATTERY
    case BMWi3:
      battery = new BMWi3Battery();
      break;
#endif

#ifdef BMW_IX_BATTERY
    case BMWIX:
      battery = new BMWIXBattery();
      break;
#endif

#ifdef BMW_PHEV_BATTERY
    case BMWPhev:
      battery = new BMWPhevBattery();
      break;
#endif

#ifdef BOLT_AMPERA_BATTERY
    case BoltAmpera:
      battery = new BoltAmperaBattery();
      break;
#endif

#ifdef BYD_ATTO_3_BATTERY
    case BydAtto3:
      battery = new BydAtto3Battery();
      break;
#endif

#ifdef CELLPOWER_BMS
    case CellPower:
      battery = new CellPowerBms();
      break;
#endif

#ifdef DALY_BMS
    case Daly:
      battery = new DalyBms();
      break;
#endif

#ifdef STELLANTIS_ECMP_BATTERY
    case Ecmp:
      battery = new StellantisEcmpBattery();
      break;
#endif

#ifdef FOXESS_BATTERY
    case Foxess:
      battery = new FoxessBattery();
      break;
#endif

#ifdef IMIEV_CZERO_ION_BATTERY
    case ImievCzeroIon:
      battery = new ImievCzeroIonBattery();
      break;
#endif

#ifdef JAGUAR_IPACE_BATTERY
    case JaguarIpace:
      battery = new JaguarIpaceBattery();
      break;
#endif

#ifdef KIA_E_GMP_BATTERY
    case KiaEGmp:
      battery = new KiaEGmpBattery();
      break;
#endif

#ifdef KIA_HYUNDAI_64_BATTERY
    case KiaHyundai64:
      battery = new KiaHyundai64Battery();
      break;
#endif

#ifdef KIA_HYUNDAI_HYBRID_BATTERY
    case KiaHyundaiHybrid:
      battery = new KiaHyundaiHybridBattery();
      break;
#endif

#ifdef MEB_BATTERY
    case Meb:
      battery = new MebBattery();
      break;
#endif

#ifdef MG_5_BATTERY
    case Mg5:
      battery = new MG5Battery();
      break;
#endif

#ifdef NISSAN_LEAF_BATTERY
    case NissanLeaf:
      battery = new NissanLeafBattery();
      break;
#endif

#ifdef PYLON_BATTERY
    case Pylon:
      battery = new PylonBattery();
      break;
#endif

#ifdef TESLA_MODEL_3Y_BATTERY
    case Tesla3Y:
      battery = new Tesla3YBattery();
      break;
#endif

#ifdef TESLA_MODEL_SX_BATTERY
    case TeslaSX:
      battery = new TeslaSXBattery();
      break;
#endif

#ifdef TEST_FAKE_BATTERY
    case TestFake:
      battery = new TestFakeBattery();
      break;
#endif

#ifdef RENAULT_ZOE_GEN1_BATTERY
    case RenaultZoeGen1:
      battery = new RenaultZoeBattery();
      break;
#endif

#ifdef RENAULT_KANGOO_BATTERY
    case RenaultKangoo:
      battery = new RenaultKangooBattery();
      break;
#endif

#ifdef RANGE_ROVER_PHEV_BATTERY
    case RangeRoverPhev:
      battery = new RangeRoverPhevBattery();
      break;
#endif
      /*case RenaultZoeGen2:
            battery = new RenaultZoeGen2Battery();
            break;*/
    default:
      return nullptr;
  }

  return battery;
}

const char* BatteryBase::name_for_type(BatteryType type) {
  switch (type) {
#ifdef BMW_I3_BATTERY
    case BMWi3:
      return BMWi3Battery::Name;
#endif

#ifdef BMW_IX_BATTERY
    case BMWIX:
      return BMWIXBattery::Name;
#endif

#ifdef BMW_PHEV_BATTERY
    case BMWPhev:
      return BMWPhevBattery::Name;
#endif

#ifdef BOLT_AMPERA_BATTERY
    case BoltAmpera:
      return BoltAmperaBattery::Name;
#endif

#ifdef BYD_ATTO_3_BATTERY
    case BydAtto3:
      return BydAtto3Battery::Name;
#endif

#ifdef CELLPOWER_BMS
    case CellPower:
      return CellPowerBms::Name;
#endif

#ifdef CHADEMO_BATTERY
    case Chademo:
      return ChademoBattery::Name;
#endif

#ifdef DALY_BMS
    case Daly:
      return DalyBms::Name;
#endif

#ifdef STELLANTIS_ECMP_BATTERY
    case Ecmp:
      return StellantisEcmpBattery::Name;
#endif

#ifdef TEST_FAKE_BATTERY
    case TestFake:
      return TestFakeBattery::Name;
#endif

#ifdef FOXESS_BATTERY
    case Foxess:
      return FoxessBattery::Name;
#endif

#ifdef IMIEV_CZERO_ION_BATTERY
    case ImievCzeroIon:
      return ImievCzeroIonBattery::Name;
#endif

#ifdef JAGUAR_IPACE_BATTERY
    case JaguarIpace:
      return JaguarIpaceBattery::Name;
#endif

#ifdef KIA_E_GMP_BATTERY
    case KiaEGmp:
      return KiaEGmpBattery::Name;
#endif

#ifdef KIA_HYUNDAI_64_BATTERY
    case KiaHyundai64:
      return KiaHyundai64Battery::Name;
#endif

#ifdef KIA_HYUNDAI_HYBRID_BATTERY
    case KiaHyundaiHybrid:
      return KiaHyundaiHybridBattery::Name;
#endif

#ifdef MEB_BATTERY
    case Meb:
      return MebBattery::Name;
#endif

#ifdef MG_5_BATTERY
    case Mg5:
      return MG5Battery::Name;
#endif

#ifdef NISSAN_LEAF_BATTERY
    case NissanLeaf:
      return NissanLeafBattery::Name;
#endif

#ifdef PYLON_BATTERY
    case Pylon:
      return PylonBattery::Name;
#endif

#ifdef TESLA_MODEL_3Y_BATTERY
    case Tesla3Y:
      return Tesla3YBattery::Name;
#endif

#ifdef TESLA_MODEL_SX_BATTERY
    case TeslaSX:
      return TeslaSXBattery::Name;
#endif

#ifdef RENAULT_ZOE_GEN1_BATTERY
    case RenaultZoeGen1:
      return RenaultZoeBattery::Name;
#endif

    default:
      return nullptr;
  }
}

std::vector<BatteryType> supported_battery_types() {
  std::vector<BatteryType> types;

  for (int i = 1; i < static_cast<int>(HighestBatteryType); i++) {
    auto type = static_cast<BatteryType>(i);
    if (CanBattery::name_for_type(type) != nullptr) {
      types.push_back(type);
    }
  }

  return types;
}
