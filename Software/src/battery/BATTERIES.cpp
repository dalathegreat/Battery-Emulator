#include "BATTERIES.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/hal/hal.h"
#include "../devboard/utils/logging.h"
#include "CanBattery.h"
#include "RS485Battery.h"

Battery* battery = nullptr;
Battery* battery2 = nullptr;
Battery* battery3 = nullptr;

std::vector<BatteryType> supported_battery_types() {
  std::vector<BatteryType> types;

  for (int i = 0; i < (int)BatteryType::Highest; i++) {
    types.push_back((BatteryType)i);
  }

  return types;
}

const char* name_for_chemistry(battery_chemistry_enum chem) {
  switch (chem) {
    case battery_chemistry_enum::Autodetect:
      return "Autodetect";
    case battery_chemistry_enum::LFP:
      return "LFP";
    case battery_chemistry_enum::NCA:
      return "NCA";
    case battery_chemistry_enum::NMC:
      return "NMC";
    case battery_chemistry_enum::ZEBRA:
      return "Molten Salt";
    default:
      return nullptr;
  }
}

const char* name_for_comm_interface(comm_interface comm) {
  return esp32hal->name_for_comm_interface(comm);
}

const char* name_for_battery_type(BatteryType type) {
  switch (type) {
    case BatteryType::None:
      return "None";

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BMW_I3)
    case BatteryType::BmwI3:
      return BmwI3Battery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BMW_IX)
    case BatteryType::BmwIX:
      return BmwIXBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BMW_PHEV)
    case BatteryType::BmwPhev:
      return BmwPhevBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BOLT_AMPERA)
    case BatteryType::BoltAmpera:
      return BoltAmperaBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BYD_ATTO3)
    case BatteryType::BydAtto3:
      return BydAttoBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_CELLPOWER)
    case BatteryType::CellPowerBms:
      return CellPowerBms::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_CHADEMO)
    case BatteryType::Chademo:
      return ChademoBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_CMFA_EV)
    case BatteryType::CmfaEv:
      return CmfaEvBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_CMP_SMART_CAR)
    case BatteryType::CmpSmartCar:
      return CmpSmartCarBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_FORD_MACH_E)
    case BatteryType::FordMachE:
      return FordMachEBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_FOXESS)
    case BatteryType::Foxess:
      return FoxessBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_GEELY_GEOMETRY_C)
    case BatteryType::GeelyGeometryC:
      return GeelyGeometryCBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_GROWATT_HV_ARK)
    case BatteryType::GrowattHvArk:
      return GrowattHvArkBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_HYUNDAI_IONIQ_28)
    case BatteryType::HyundaiIoniq28:
      return HyundaiIoniq28Battery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_ORION)
    case BatteryType::OrionBms:
      return OrionBms::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_SONO)
    case BatteryType::Sono:
      return SonoBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_ECMP)
    case BatteryType::StellantisEcmp:
      return EcmpBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_IMIEV_CZERO)
    case BatteryType::ImievCZeroIon:
      return ImievCZeroIonBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_JAGUAR_IPACE)
    case BatteryType::JaguarIpace:
      return JaguarIpaceBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_KIA_E_GMP)
    case BatteryType::KiaEGmp:
      return KiaEGmpBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_KIA_HYUNDAI_64)
    case BatteryType::KiaHyundai64:
      return KiaHyundai64Battery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_KIA_64FD)
    case BatteryType::Kia64FD:
      return Kia64FDBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_KIA_HYUNDAI_HYBRID)
    case BatteryType::KiaHyundaiHybrid:
      return KiaHyundaiHybridBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_MEB)
    case BatteryType::Meb:
      return MebBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_MG_5)
#ifndef SMALL_FLASH_DEVICE
    case BatteryType::Mg5:
      return Mg5Battery::Name;
#endif
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_MG_HS_PHEV)
    case BatteryType::MgHsPhev:
      return MgHsPHEVBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_NISSAN_LEAF)
    case BatteryType::NissanLeaf:
      return NissanLeafBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_PYLON)
    case BatteryType::Pylon:
      return PylonBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_DALY)
    case BatteryType::DalyBms:
      return DalyBms::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RJXZS)
    case BatteryType::RjxzsBms:
      return RjxzsBms::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RANGE_ROVER_PHEV)
    case BatteryType::RangeRoverPhev:
      return RangeRoverPhevBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RELION_LV)
    case BatteryType::RelionBattery:
      return RelionBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RENAULT_KANGOO)
    case BatteryType::RenaultKangoo:
      return RenaultKangooBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RENAULT_TWIZY)
    case BatteryType::RenaultTwizy:
      return RenaultTwizyBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RENAULT_ZOE_GEN1)
    case BatteryType::RenaultZoe1:
      return RenaultZoeGen1Battery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RENAULT_ZOE_GEN2)
    case BatteryType::RenaultZoe2:
      return RenaultZoeGen2Battery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RIVIAN)
    case BatteryType::RivianBattery:
      return RivianBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_SAMSUNG_SDI_LV)
    case BatteryType::SamsungSdiLv:
      return SamsungSdiLVBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_SANTA_FE_PHEV)
    case BatteryType::SantaFePhev:
      return SantaFePhevBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_SIMPBMS)
    case BatteryType::SimpBms:
      return SimpBmsBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_TESLA)
    case BatteryType::TeslaModel3Y:
      return TeslaModel3YBattery::Name;
    case BatteryType::TeslaModelSX:
      return TeslaModelSXBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_TESLA_LEGACY)
    case BatteryType::TeslaLegacy:
      return TeslaLegacyBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_TEST_FAKE)
    case BatteryType::TestFake:
      return TestFakeBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_THINK)
    case BatteryType::ThinkCity:
      return ThinkBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_THUNDERSTRUCK)
    case BatteryType::ThunderstruckBMS:
      return ThunderstruckBMS::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_GEELY_SEA)
    case BatteryType::GeelySea:
      return GeelySeaBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_VOLVO_SPA)
    case BatteryType::VolvoSpa:
      return VolvoSpaBattery::Name;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_VOLVO_SPA_HYBRID)
    case BatteryType::VolvoSpaHybrid:
      return VolvoSpaHybridBattery::Name;
#endif

    default:
      return nullptr;
  }
}

const battery_chemistry_enum battery_chemistry_default = battery_chemistry_enum::NMC;

battery_chemistry_enum user_selected_battery_chemistry = battery_chemistry_default;

BatteryType user_selected_battery_type = BatteryType::None;
bool user_selected_second_battery = false;
bool user_selected_triple_battery = false;

Battery* create_battery(BatteryType type) {
  switch (type) {
    case BatteryType::None:
      return nullptr;

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BMW_I3)
    case BatteryType::BmwI3:
      return new BmwI3Battery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BMW_IX)
    case BatteryType::BmwIX:
      return new BmwIXBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BMW_PHEV)
    case BatteryType::BmwPhev:
      return new BmwPhevBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BOLT_AMPERA)
    case BatteryType::BoltAmpera:
      return new BoltAmperaBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BYD_ATTO3)
    case BatteryType::BydAtto3:
      return new BydAttoBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_CELLPOWER)
    case BatteryType::CellPowerBms:
      return new CellPowerBms();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_CHADEMO)
    case BatteryType::Chademo:
      return new ChademoBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_CMFA_EV)
    case BatteryType::CmfaEv:
      return new CmfaEvBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_CMP_SMART_CAR)
    case BatteryType::CmpSmartCar:
      return new CmpSmartCarBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_FORD_MACH_E)
    case BatteryType::FordMachE:
      return new FordMachEBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_FOXESS)
    case BatteryType::Foxess:
      return new FoxessBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_GEELY_GEOMETRY_C)
    case BatteryType::GeelyGeometryC:
      return new GeelyGeometryCBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_GROWATT_HV_ARK)
    case BatteryType::GrowattHvArk:
      return new GrowattHvArkBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_HYUNDAI_IONIQ_28)
    case BatteryType::HyundaiIoniq28:
      return new HyundaiIoniq28Battery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_ORION)
    case BatteryType::OrionBms:
      return new OrionBms();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_SONO)
    case BatteryType::Sono:
      return new SonoBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_ECMP)
    case BatteryType::StellantisEcmp:
      return new EcmpBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_IMIEV_CZERO)
    case BatteryType::ImievCZeroIon:
      return new ImievCZeroIonBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_JAGUAR_IPACE)
    case BatteryType::JaguarIpace:
      return new JaguarIpaceBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_KIA_64FD)
    case BatteryType::Kia64FD:
      return new Kia64FDBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_KIA_E_GMP)
    case BatteryType::KiaEGmp:
      return new KiaEGmpBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_KIA_HYUNDAI_64)
    case BatteryType::KiaHyundai64:
      return new KiaHyundai64Battery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_KIA_HYUNDAI_HYBRID)
    case BatteryType::KiaHyundaiHybrid:
      return new KiaHyundaiHybridBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_MEB)
    case BatteryType::Meb:
      return new MebBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_MG_5)
#ifndef SMALL_FLASH_DEVICE
    case BatteryType::Mg5:
      return new Mg5Battery();
#endif
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_MG_HS_PHEV)
    case BatteryType::MgHsPhev:
      return new MgHsPHEVBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_NISSAN_LEAF)
    case BatteryType::NissanLeaf:
      return new NissanLeafBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_PYLON)
    case BatteryType::Pylon:
      return new PylonBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_DALY)
    case BatteryType::DalyBms:
      return new DalyBms();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RJXZS)
    case BatteryType::RjxzsBms:
      return new RjxzsBms();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RANGE_ROVER_PHEV)
    case BatteryType::RangeRoverPhev:
      return new RangeRoverPhevBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RELION_LV)
    case BatteryType::RelionBattery:
      return new RelionBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RENAULT_KANGOO)
    case BatteryType::RenaultKangoo:
      return new RenaultKangooBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RENAULT_TWIZY)
    case BatteryType::RenaultTwizy:
      return new RenaultTwizyBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RENAULT_ZOE_GEN1)
    case BatteryType::RenaultZoe1:
      return new RenaultZoeGen1Battery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RENAULT_ZOE_GEN2)
    case BatteryType::RenaultZoe2:
      return new RenaultZoeGen2Battery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RIVIAN)
    case BatteryType::RivianBattery:
      return new RivianBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_SAMSUNG_SDI_LV)
    case BatteryType::SamsungSdiLv:
      return new SamsungSdiLVBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_SANTA_FE_PHEV)
    case BatteryType::SantaFePhev:
      return new SantaFePhevBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_SIMPBMS)
    case BatteryType::SimpBms:
      return new SimpBmsBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_TESLA)
    case BatteryType::TeslaModel3Y:
      return new TeslaModel3YBattery();
    case BatteryType::TeslaModelSX:
      return new TeslaModelSXBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_TESLA_LEGACY)
    case BatteryType::TeslaLegacy:
      return new TeslaLegacyBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_TEST_FAKE)
    case BatteryType::TestFake:
      return new TestFakeBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_THINK)
    case BatteryType::ThinkCity:
      return new ThinkBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_THUNDERSTRUCK)
    case BatteryType::ThunderstruckBMS:
      return new ThunderstruckBMS();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_GEELY_SEA)
    case BatteryType::GeelySea:
      return new GeelySeaBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_VOLVO_SPA)
    case BatteryType::VolvoSpa:
      return new VolvoSpaBattery();
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_VOLVO_SPA_HYBRID)
    case BatteryType::VolvoSpaHybrid:
      return new VolvoSpaHybridBattery();
#endif

    default:
      return nullptr;
  }
}

void setup_battery() {
  if (battery) {
    // Let's not create the battery again.
    return;
  }

  // Set the chemistry to the user selected value, the battery can override.
  datalayer.battery.info.chemistry = user_selected_battery_chemistry;
  datalayer.battery2.info.chemistry = user_selected_battery_chemistry;
  datalayer.battery3.info.chemistry = user_selected_battery_chemistry;

  battery = create_battery(user_selected_battery_type);

  if (battery) {
    battery->setup();
  }

  if (user_selected_second_battery && !battery2) {
    switch (user_selected_battery_type) {

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BYD_ATTO3)
      case BatteryType::BydAtto3:
        battery2 = new BydAttoBattery(&datalayer.battery2, nullptr, can_config.battery_double);
        break;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_NISSAN_LEAF)
      case BatteryType::NissanLeaf:
        battery2 = new NissanLeafBattery(&datalayer.battery2, nullptr, can_config.battery_double);
        break;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BMW_I3)
      case BatteryType::BmwI3:
        battery2 = new BmwI3Battery(&datalayer.battery2, &datalayer.system.status.battery2_allowed_contactor_closing,
                                    can_config.battery_double, esp32hal->WUP_PIN2());
        break;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_CMFA_EV)
      case BatteryType::CmfaEv:
        battery2 = new CmfaEvBattery(&datalayer.battery2, nullptr, can_config.battery_double);
        break;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_CMP_SMART_CAR)
      case BatteryType::CmpSmartCar:
        battery2 = new CmpSmartCarBattery(&datalayer.battery2, nullptr, can_config.battery_double);
        break;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_KIA_HYUNDAI_64)
      case BatteryType::KiaHyundai64:
        battery2 = new KiaHyundai64Battery(&datalayer.battery2, &datalayer_extended.KiaHyundai64_2,
                                           &datalayer.system.status.battery2_allowed_contactor_closing,
                                           can_config.battery_double);
        break;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_PYLON)
      case BatteryType::Pylon:
        battery2 = new PylonBattery(&datalayer.battery2, nullptr, can_config.battery_double);
        break;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_SANTA_FE_PHEV)
      case BatteryType::SantaFePhev:
        battery2 = new SantaFePhevBattery(&datalayer.battery2, can_config.battery_double);
        break;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RELION_LV)
      case BatteryType::RelionBattery:
        battery2 = new RelionBattery(&datalayer.battery2, can_config.battery_double);
        break;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RENAULT_ZOE_GEN1)
      case BatteryType::RenaultZoe1:
        battery2 = new RenaultZoeGen1Battery(&datalayer.battery2, nullptr, can_config.battery_double);
        break;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RENAULT_ZOE_GEN2)
      case BatteryType::RenaultZoe2:
        battery2 = new RenaultZoeGen2Battery(&datalayer.battery2, nullptr, can_config.battery_double);
        break;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_TEST_FAKE)
      case BatteryType::TestFake:
        battery2 = new TestFakeBattery(&datalayer.battery2, can_config.battery_double);
        break;
#endif

      default:
        DEBUG_PRINTF("User tried enabling double battery on non-supported integration!\n");
        break;
    }

    if (battery2) {
      battery2->setup();
    }
  }

  if (user_selected_triple_battery && !battery3) {
    switch (user_selected_battery_type) {

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_NISSAN_LEAF)
      case BatteryType::NissanLeaf:
        battery3 = new NissanLeafBattery(&datalayer.battery3, nullptr, can_config.battery_triple);
        break;
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RELION_LV)
      case BatteryType::RelionBattery:
        battery3 = new RelionBattery(&datalayer.battery3, can_config.battery_triple);
        break;
#endif

      default:
        DEBUG_PRINTF("User tried enabling triple battery on non-supported integration!\n");
        break;
    }

    if (battery3) {
      battery3->setup();
    }
  }
}

/* User-selected Nissan LEAF settings */
bool user_selected_LEAF_interlock_mandatory = false;
/* User-selected Tesla settings */
bool user_selected_tesla_digital_HVIL = false;
uint16_t user_selected_tesla_GTW_country = 17477;
bool user_selected_tesla_GTW_rightHandDrive = true;
uint16_t user_selected_tesla_GTW_mapRegion = 2;
uint16_t user_selected_tesla_GTW_chassisType = 2;
uint16_t user_selected_tesla_GTW_packEnergy = 1;
/* User-selected EGMP+others settings */
bool user_selected_use_estimated_SOC = false;
uint16_t user_selected_pylon_baudrate = 500;

// Use 0V for user selected cell/pack voltage defaults (On boot will be replaced with saved values from NVM)
uint16_t user_selected_max_pack_voltage_dV = 0;
uint16_t user_selected_min_pack_voltage_dV = 0;
uint16_t user_selected_max_cell_voltage_mV = 0;
uint16_t user_selected_min_cell_voltage_mV = 0;