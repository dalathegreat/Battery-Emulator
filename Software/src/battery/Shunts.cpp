#include "../include.h"
#include "Shunt.h"

CanShunt* shunt = nullptr;
ShuntType user_selected_shunt_type = ShuntType::None;

#ifdef COMMON_IMAGE
#ifdef SELECTED_SHUNT_CLASS
#error "Compile time SELECTED_SHUNT_CLASS should not be defined with COMMON_IMAGE"
#endif

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

#else
void setup_can_shunt() {
  if (shunt) {
    return;
  }

#if defined(SELECTED_SHUNT_CLASS)
  shunt = new SELECTED_SHUNT_CLASS();
  if (shunt) {
    shunt->setup();
  }
#endif
}
#endif

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
