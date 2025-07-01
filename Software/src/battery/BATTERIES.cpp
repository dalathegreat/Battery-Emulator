#include "../include.h"

#include "../datalayer/datalayer_extended.h"
#include "CanBattery.h"
#include "RS485Battery.h"

#if !defined(COMMON_IMAGE) && !defined(SELECTED_BATTERY_CLASS)
#error No battery selected! Choose one from the USER_SETTINGS.h file
#endif

Battery* battery = nullptr;
Battery* battery2 = nullptr;

std::vector<BatteryType> supported_battery_types() {
  std::vector<BatteryType> types;

  for (int i = 0; i < (int)BatteryType::Highest; i++) {
    types.push_back((BatteryType)i);
  }

  return types;
}

extern const char* name_for_battery_type(BatteryType type) {
  switch (type) {
    case BatteryType::None:
      return "None";
    case BatteryType::BmwI3:
      return BmwI3Battery::Name;
    case BatteryType::BmwIx:
      return BmwIXBattery::Name;
    case BatteryType::BoltAmpera:
      return BoltAmperaBattery::Name;
    case BatteryType::BydAtto3:
      return BydAttoBattery::Name;
    case BatteryType::CellPowerBms:
      return CellPowerBms::Name;
#ifdef CHADEMO_PIN_2  // Only support chademo for certain platforms
    case BatteryType::Chademo:
      return ChademoBattery::Name;
#endif
    case BatteryType::CmfaEv:
      return CmfaEvBattery::Name;
    case BatteryType::Foxess:
      return FoxessBattery::Name;
    case BatteryType::GeelyGeometryC:
      return GeelyGeometryCBattery::Name;
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
    case BatteryType::KiaHyundaiHybrid:
      return KiaHyundaiHybridBattery::Name;
    case BatteryType::Meb:
      return MebBattery::Name;
    case BatteryType::Mg5:
      return Mg5Battery::Name;
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
    case BatteryType::RenaultKangoo:
      return RenaultKangooBattery::Name;
    case BatteryType::RenaultTwizy:
      return RenaultTwizyBattery::Name;
    case BatteryType::RenaultZoe1:
      return RenaultZoeGen1Battery::Name;
    case BatteryType::RenaultZoe2:
      return RenaultZoeGen2Battery::Name;
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

#ifdef COMMON_IMAGE
#ifdef SELECTED_BATTERY_CLASS
#error "Compile time SELECTED_BATTERY_CLASS should not be defined with COMMON_IMAGE"
#endif

BatteryType user_selected_battery_type = BatteryType::NissanLeaf;
bool user_selected_second_battery = false;

Battery* create_battery(BatteryType type) {
  switch (type) {
    case BatteryType::None:
      return nullptr;
    case BatteryType::BmwI3:
      return new BmwI3Battery();
    case BatteryType::BmwIx:
      return new BmwIXBattery();
    case BatteryType::BoltAmpera:
      return new BoltAmperaBattery();
    case BatteryType::BydAtto3:
      return new BydAttoBattery();
    case BatteryType::CellPowerBms:
      return new CellPowerBms();
#ifdef CHADEMO_PIN_2  // Only support chademo for certain platforms
    case BatteryType::Chademo:
      return new ChademoBattery();
#endif
    case BatteryType::CmfaEv:
      return new CmfaEvBattery();
    case BatteryType::Foxess:
      return new FoxessBattery();
    case BatteryType::GeelyGeometryC:
      return new GeelyGeometryCBattery();
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
    case BatteryType::RenaultKangoo:
      return new RenaultKangooBattery();
    case BatteryType::RenaultTwizy:
      return new RenaultTwizyBattery();
    case BatteryType::RenaultZoe1:
      return new RenaultZoeGen1Battery();
    case BatteryType::RenaultZoe2:
      return new RenaultZoeGen2Battery();
    case BatteryType::SantaFePhev:
      return new SantaFePhevBattery();
    case BatteryType::SimpBms:
      return new SimpBmsBattery();
    case BatteryType::TeslaModel3Y:
      return new TeslaModel3YBattery();
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
                                    can_config.battery_double, WUP_PIN2);
        break;
      case BatteryType::KiaHyundai64:
        battery2 = new KiaHyundai64Battery(&datalayer.battery2, &datalayer_extended.KiaHyundai64_2,
                                           &datalayer.system.status.battery2_allowed_contactor_closing,
                                           can_config.battery_double);
      case BatteryType::SantaFePhev:
        battery2 = new SantaFePhevBattery(&datalayer.battery2, can_config.battery_double);
        break;
      case BatteryType::TestFake:
        battery2 = new TestFakeBattery(&datalayer.battery2, can_config.battery_double);
        break;
    }

    if (battery2) {
      battery2->setup();
    }
  }
}
#else  // Battery selection has been made at build-time

void setup_battery() {
  // Instantiate the battery only once just in case this function gets called multiple times.
  if (battery == nullptr) {
    battery = new SELECTED_BATTERY_CLASS();
  }
  battery->setup();

#ifdef DOUBLE_BATTERY
  if (battery2 == nullptr) {
#if defined(BMW_I3_BATTERY)
    battery2 =
        new SELECTED_BATTERY_CLASS(&datalayer.battery2, &datalayer.system.status.battery2_allowed_contactor_closing,
                                   can_config.battery_double, WUP_PIN2);
#elif defined(KIA_HYUNDAI_64_BATTERY)
    battery2 = new SELECTED_BATTERY_CLASS(&datalayer.battery2, &datalayer_extended.KiaHyundai64_2,
                                          &datalayer.system.status.battery2_allowed_contactor_closing,
                                          can_config.battery_double);
#elif defined(SANTA_FE_PHEV_BATTERY) || defined(TEST_FAKE_BATTERY)
    battery2 = new SELECTED_BATTERY_CLASS(&datalayer.battery2, can_config.battery_double);
#else
    battery2 = new SELECTED_BATTERY_CLASS(&datalayer.battery2, nullptr, can_config.battery_double);
#endif
  }
  battery2->setup();
#endif
}
#endif
