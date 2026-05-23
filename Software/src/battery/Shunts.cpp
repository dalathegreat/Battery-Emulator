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

extern std::vector<ShuntType> supported_shunt_types() {
  std::vector<ShuntType> types;
  types.push_back(ShuntType::None);
  types.push_back(ShuntType::BmwSbox);

  if (inverter && inverter->provides_shunt())
    types.push_back(ShuntType::Inverter);

  types.push_back(ShuntType::CustomClamp);
  return types;
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
