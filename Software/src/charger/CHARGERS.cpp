#include "CHARGERS.h"
#include <vector>
#include "CanCharger.h"

CanCharger* charger = nullptr;

ChargerType user_selected_charger_type = ChargerType::None;

/* Charger settings (Optional, when using generator charging) */
//TODO: These should be user configurable via webserver
volatile float CHARGER_SET_HV = 384;      // Reasonably appropriate 4.0v per cell charging of a 96s pack
volatile float CHARGER_MAX_HV = 420;      // Max permissible output (VDC) of charger
volatile float CHARGER_MIN_HV = 200;      // Min permissible output (VDC) of charger
volatile float CHARGER_MAX_POWER = 3300;  // Max power capable of charger, as a ceiling for validating config
volatile float CHARGER_MAX_A = 11.5;      // Max current output (amps) of charger
volatile float CHARGER_END_A = 1.0;       // Current at which charging is considered complete

std::vector<ChargerType> supported_charger_types() {
  std::vector<ChargerType> types;

  for (int i = 0; i < (int)ChargerType::Highest; i++) {
    types.push_back((ChargerType)i);
  }

  return types;
}

extern const char* name_for_charger_type(ChargerType type) {
  switch (type) {
    case ChargerType::ChevyVolt:
      return ChevyVoltCharger::Name;
    case ChargerType::NissanLeaf:
      return NissanLeafCharger::Name;
    case ChargerType::None:
    case ChargerType::Highest:
      return "None";
  }

  return nullptr;
}

void setup_charger() {

  switch (user_selected_charger_type) {
    case ChargerType::ChevyVolt:
      charger = new ChevyVoltCharger();
      break;
    case ChargerType::NissanLeaf:
      charger = new NissanLeafCharger();
      break;
    case ChargerType::None:
    case ChargerType::Highest:
      break;
  }
}
