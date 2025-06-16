#include "../include.h"

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

extern const char* name_for_type(BatteryType type) {
  switch (type) {
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
    case BatteryType::Chademo:
      return ChademoBattery::Name;
    case BatteryType::NissanLeaf:
      return NissanLeafBattery::Name;
    case BatteryType::TestFake:
      return TestFakeBattery::Name;
    case BatteryType::TeslaModel3Y:
      return TeslaModel3YBattery::Name;
    case BatteryType::TeslaModelSX:
      return TeslaModelSXBattery::Name;
    case BatteryType::None:
      return "None";
  }

  return nullptr;
}

#ifdef COMMON_IMAGE
#ifdef SELECTED_BATTERY_CLASS
#error "Compile time SELECTED_BATTERY_CLASS should not be defined with COMMON_IMAGE"
#endif

BatteryType user_selected_battery_type = BatteryType::NissanLeaf;
bool user_selected_second_battery = false;

void setup_battery() {

  if (battery) {
    // Let's not create the battery again.
    return;
  }

  switch (user_selected_battery_type) {
    case BatteryType::BmwI3:
      battery = new BmwI3Battery();
      break;
    case BatteryType::BmwIx:
      battery = new BmwIXBattery();
      break;
    case BatteryType::BoltAmpera:
      battery = new BoltAmperaBattery();
      break;
    case BatteryType::BydAtto3:
      battery = new BydAttoBattery();
      break;
    case BatteryType::CellPowerBms:
      battery = new CellPowerBms();
      break;
    case BatteryType::Chademo:
      battery = new ChademoBattery();
      break;
    case BatteryType::CmfaEv:
      battery = new CmfaEvBattery();
      break;
    case BatteryType::DalyBms:
      battery = new DalyBms();
      break;
    case BatteryType::NissanLeaf:
      battery = new NissanLeafBattery();
      break;
    case BatteryType::TeslaModel3Y:
      battery = new TeslaModel3YBattery();
      break;
    case BatteryType::TeslaModelSX:
      battery = new TeslaModelSXBattery();
      break;
    case BatteryType::TestFake:
      battery = new TestFakeBattery();
      break;
  }

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
