#include "BATTERIES.h"
#include "../datalayer/datalayer_extended.h"
#include "CanBattery.h"
#include "RS485Battery.h"

Battery* battery = nullptr;
Battery* battery2 = nullptr;

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
    case BatteryType::BmwI3:
      return BmwI3Battery::Name;
    case BatteryType::BmwIX:
      return BmwIXBattery::Name;
    case BatteryType::BmwPhev:
      return BmwPhevBattery::Name;
    case BatteryType::BoltAmpera:
      return BoltAmperaBattery::Name;
    case BatteryType::BydAtto3:
      return BydAttoBattery::Name;
    case BatteryType::CellPowerBms:
      return CellPowerBms::Name;
    case BatteryType::Chademo:
      return ChademoBattery::Name;
    case BatteryType::CmfaEv:
      return CmfaEvBattery::Name;
    case BatteryType::Foxess:
      return FoxessBattery::Name;
    case BatteryType::GeelyGeometryC:
      return GeelyGeometryCBattery::Name;
    case BatteryType::HyundaiIoniq28:
      return HyundaiIoniq28Battery::Name;
    case BatteryType::OrionBms:
      return OrionBms::Name;
    case BatteryType::Sono:
      return SonoBattery::Name;
    case BatteryType::StellantisEcmp:
      return EcmpBattery::Name;
    case BatteryType::ImievCZeroIon:
      return ImievCZeroIonBattery::Name;
    case BatteryType::JaguarIpace:
      return JaguarIpaceBattery::Name;
    case BatteryType::KiaEGmp:
      return KiaEGmpBattery::Name;
    case BatteryType::KiaHyundai64:
      return KiaHyundai64Battery::Name;
    case BatteryType::Kia64FD:
      return Kia64FDBattery::Name;
    case BatteryType::KiaHyundaiHybrid:
      return KiaHyundaiHybridBattery::Name;
    case BatteryType::Meb:
      return MebBattery::Name;
    case BatteryType::Mg5:
      return Mg5Battery::Name;
    case BatteryType::MgHsPhev:
      return MgHsPHEVBattery::Name;
    case BatteryType::NissanLeaf:
      return NissanLeafBattery::Name;
    case BatteryType::Pylon:
      return PylonBattery::Name;
    case BatteryType::DalyBms:
      return DalyBms::Name;
    case BatteryType::RjxzsBms:
      return RjxzsBms::Name;
    case BatteryType::RangeRoverPhev:
      return RangeRoverPhevBattery::Name;
    case BatteryType::RelionBattery:
      return RelionBattery::Name;
    case BatteryType::RenaultKangoo:
      return RenaultKangooBattery::Name;
    case BatteryType::RenaultTwizy:
      return RenaultTwizyBattery::Name;
    case BatteryType::RenaultZoe1:
      return RenaultZoeGen1Battery::Name;
    case BatteryType::RenaultZoe2:
      return RenaultZoeGen2Battery::Name;
    case BatteryType::RivianBattery:
      return RivianBattery::Name;
    case BatteryType::SamsungSdiLv:
      return SamsungSdiLVBattery::Name;
    case BatteryType::SantaFePhev:
      return SantaFePhevBattery::Name;
    case BatteryType::SimpBms:
      return SimpBmsBattery::Name;
    case BatteryType::TeslaModel3Y:
      return TeslaModel3YBattery::Name;
    case BatteryType::TeslaModelSX:
      return TeslaModelSXBattery::Name;
    case BatteryType::TestFake:
      return TestFakeBattery::Name;
    case BatteryType::VolvoSpa:
      return VolvoSpaBattery::Name;
    case BatteryType::VolvoSpaHybrid:
      return VolvoSpaHybridBattery::Name;
    default:
      return nullptr;
  }
}

const battery_chemistry_enum battery_chemistry_default = battery_chemistry_enum::NMC;

battery_chemistry_enum user_selected_battery_chemistry = battery_chemistry_default;

BatteryType user_selected_battery_type = BatteryType::NissanLeaf;
bool user_selected_second_battery = false;

Battery* create_battery(BatteryType type) {
  switch (type) {
    case BatteryType::None:
      return nullptr;
    case BatteryType::BmwI3:
      return new BmwI3Battery();
    case BatteryType::BmwIX:
      return new BmwIXBattery();
    case BatteryType::BmwPhev:
      return new BmwPhevBattery();
    case BatteryType::BoltAmpera:
      return new BoltAmperaBattery();
    case BatteryType::BydAtto3:
      return new BydAttoBattery();
    case BatteryType::CellPowerBms:
      return new CellPowerBms();
    case BatteryType::Chademo:
      return new ChademoBattery();
    case BatteryType::CmfaEv:
      return new CmfaEvBattery();
    case BatteryType::Foxess:
      return new FoxessBattery();
    case BatteryType::GeelyGeometryC:
      return new GeelyGeometryCBattery();
    case BatteryType::HyundaiIoniq28:
      return new HyundaiIoniq28Battery();
    case BatteryType::OrionBms:
      return new OrionBms();
    case BatteryType::Sono:
      return new SonoBattery();
    case BatteryType::StellantisEcmp:
      return new EcmpBattery();
    case BatteryType::ImievCZeroIon:
      return new ImievCZeroIonBattery();
    case BatteryType::JaguarIpace:
      return new JaguarIpaceBattery();
    case BatteryType::Kia64FD:
      return new Kia64FDBattery();
    case BatteryType::KiaEGmp:
      return new KiaEGmpBattery();
    case BatteryType::KiaHyundai64:
      return new KiaHyundai64Battery();
    case BatteryType::KiaHyundaiHybrid:
      return new KiaHyundaiHybridBattery();
    case BatteryType::Meb:
      return new MebBattery();
    case BatteryType::Mg5:
      return new Mg5Battery();
    case BatteryType::MgHsPhev:
      return new MgHsPHEVBattery();
    case BatteryType::NissanLeaf:
      return new NissanLeafBattery();
    case BatteryType::Pylon:
      return new PylonBattery();
    case BatteryType::DalyBms:
      return new DalyBms();
    case BatteryType::RjxzsBms:
      return new RjxzsBms();
    case BatteryType::RangeRoverPhev:
      return new RangeRoverPhevBattery();
    case BatteryType::RelionBattery:
      return new RelionBattery();
    case BatteryType::RenaultKangoo:
      return new RenaultKangooBattery();
    case BatteryType::RenaultTwizy:
      return new RenaultTwizyBattery();
    case BatteryType::RenaultZoe1:
      return new RenaultZoeGen1Battery();
    case BatteryType::RenaultZoe2:
      return new RenaultZoeGen2Battery();
    case BatteryType::RivianBattery:
      return new RivianBattery();
    case BatteryType::SamsungSdiLv:
      return new SamsungSdiLVBattery();
    case BatteryType::SantaFePhev:
      return new SantaFePhevBattery();
    case BatteryType::SimpBms:
      return new SimpBmsBattery();
    case BatteryType::TeslaModel3Y:
      return new TeslaModel3YBattery(user_selected_battery_chemistry);
    case BatteryType::TeslaModelSX:
      return new TeslaModelSXBattery();
    case BatteryType::TestFake:
      return new TestFakeBattery();
    case BatteryType::VolvoSpa:
      return new VolvoSpaBattery();
    case BatteryType::VolvoSpaHybrid:
      return new VolvoSpaHybridBattery();
    default:
      return nullptr;
  }
}

void setup_battery() {
  if (battery) {
    // Let's not create the battery again.
    return;
  }

  battery = create_battery(user_selected_battery_type);

  if (battery) {
    battery->setup();
  }

  if (user_selected_second_battery && !battery2) {
    switch (user_selected_battery_type) {
      case BatteryType::NissanLeaf:
        battery2 = new NissanLeafBattery(&datalayer.battery2, nullptr, can_config.battery_double);
        break;
      case BatteryType::BmwI3:
        battery2 = new BmwI3Battery(&datalayer.battery2, &datalayer.system.status.battery2_allowed_contactor_closing,
                                    can_config.battery_double, esp32hal->WUP_PIN2());
        break;
      case BatteryType::CmfaEv:
        battery2 = new CmfaEvBattery(&datalayer.battery2, nullptr, can_config.battery_double);
        break;
      case BatteryType::KiaHyundai64:
        battery2 = new KiaHyundai64Battery(&datalayer.battery2, &datalayer_extended.KiaHyundai64_2,
                                           &datalayer.system.status.battery2_allowed_contactor_closing,
                                           can_config.battery_double);
        break;
      case BatteryType::SantaFePhev:
        battery2 = new SantaFePhevBattery(&datalayer.battery2, can_config.battery_double);
        break;
      case BatteryType::RenaultZoe1:
        battery2 = new RenaultZoeGen1Battery(&datalayer.battery2, nullptr, can_config.battery_double);
        break;
      case BatteryType::RenaultZoe2:
        battery2 = new RenaultZoeGen2Battery(&datalayer.battery2, nullptr, can_config.battery_double);
        break;
      case BatteryType::TestFake:
        battery2 = new TestFakeBattery(&datalayer.battery2, can_config.battery_double);
        break;
      default:
        DEBUG_PRINTF("User tried enabling double battery on non-supported integration!\n");
        break;
    }

    if (battery2) {
      battery2->setup();
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

// Use 0V for user selected cell/pack voltage defaults (On boot will be replaced with saved values from NVM)
uint16_t user_selected_max_pack_voltage_dV = 0;
uint16_t user_selected_min_pack_voltage_dV = 0;
uint16_t user_selected_max_cell_voltage_mV = 0;
uint16_t user_selected_min_cell_voltage_mV = 0;
