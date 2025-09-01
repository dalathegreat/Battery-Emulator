#include "BMW-SBOX.h"
#include "Shunt.h"

CanShunt* shunt = nullptr;
ShuntType user_selected_shunt_type = ShuntType::None;

void setup_can_shunt() {
  if (shunt) {
    return;
  }

  switch (user_selected_shunt_type) {
    case ShuntType::None:
      shunt = nullptr;
      return;
    case ShuntType::BmwSbox:
      shunt = new BmwSbox();
      break;
    default:
      return;
  }

  if (shunt) {
    shunt->setup();
  }
}

extern std::vector<ShuntType> supported_shunt_types() {
  std::vector<ShuntType> types;

  for (int i = 0; i < (int)ShuntType::Highest; i++) {
    types.push_back((ShuntType)i);
  }

  return types;
}

extern const char* name_for_shunt_type(ShuntType type) {
  switch (type) {
    case ShuntType::None:
      return "None";
    case ShuntType::BmwSbox:
      return BmwSbox::Name;
    default:
      return nullptr;
  }
}
