#ifndef CHARGERS_H
#define CHARGERS_H
#include "../../USER_SETTINGS.h"

#include "CHEVY-VOLT-CHARGER.h"
#include "NISSAN-LEAF-CHARGER.h"

// Constructs the global charger object based on build-time selection of charger type.
// Safe to call even though no charger is selected.
void setup_charger();

// The selected charger or null if no charger in use.
extern CanCharger* charger;

#endif
