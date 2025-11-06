#ifndef CHARGERS_H
#define CHARGERS_H

#include "CHEVY-VOLT-CHARGER.h"
#include "NISSAN-LEAF-CHARGER.h"

// Constructs the global charger object based on build-time selection of charger type.
// Safe to call even though no charger is selected.
void setup_charger();

// The selected charger or null if no charger in use.
extern CanCharger* charger;

//TODO: These should be user configurable via webserver
extern volatile float CHARGER_SET_HV;
extern volatile float CHARGER_MAX_HV;
extern volatile float CHARGER_MIN_HV;
extern volatile float CHARGER_MAX_POWER;
extern volatile float CHARGER_MAX_A;
extern volatile float CHARGER_END_A;

#endif
