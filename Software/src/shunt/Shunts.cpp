#include "../battery/Battery.h"
#include "../inverter/INVERTERS.h"
#include "BMW-SBOX.h"
#include "Shunt.h"

CanShunt* shunt = nullptr;
ShuntType user_selected_shunt_type = ShuntType::None;

void setup_shunt() {
  if (shunt) {
    return;
  }

  switch (user_selected_shunt_type) {
    case ShuntType::None:
      shunt = nullptr;
      return;
    case ShuntType::BmwSbox:
      shunt = new BmwSbox();
      shunt->setup();
      return;
    case ShuntType::Inverter:
      if (inverter && inverter->provides_shunt()) {
        inverter->enable_shunt();
      }
      return;
    case ShuntType::CustomClamp:
      shunt = nullptr;
      return;
    default:
      return;
  }
}

bool shunt_type_supported_by_battery(ShuntType type, BatteryType battery_type) {
  if (type == ShuntType::CustomClamp) {
    return battery_type == BatteryType::Chademo;
  }
  return true;
}

bool shunt_type_supported_by_inverter(ShuntType type, InverterProtocolType inverter_type) {
  if (type == ShuntType::Inverter) {
    return inverter_type_provides_shunt(inverter_type);
  }
  return true;
}

extern const char* name_for_shunt_type(ShuntType type) {
  switch (type) {
    case ShuntType::None:
      return "None";
    case ShuntType::BmwSbox:
      return BmwSbox::Name;
    case ShuntType::Inverter:
      return "Using inverter values";
    case ShuntType::CustomClamp:
      return "Custom Clamp";
    default:
      return nullptr;
  }
}
