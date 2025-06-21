#include "../include.h"
#include "Shunt.h"

CanShunt* shunt = nullptr;

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
