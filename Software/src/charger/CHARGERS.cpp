#include "../include.h"

CanCharger* charger = nullptr;

void setup_charger() {
#ifdef SELECTED_CHARGER_CLASS
  charger = new SELECTED_CHARGER_CLASS();
#endif
}
