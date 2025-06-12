#ifndef INVERTERS_H
#define INVERTERS_H

#include "InverterProtocol.h"
extern InverterProtocol* inverter;

#include "../../USER_SETTINGS.h"

#include "AFORE-CAN.h"

#ifdef BYD_CAN_DEYE
#define BYD_CAN
#endif

#include "BYD-CAN.h"
#include "BYD-MODBUS.h"
#include "FERROAMP-CAN.h"
#include "FOXESS-CAN.h"
#include "GROWATT-HV-CAN.h"
#include "GROWATT-LV-CAN.h"
#include "KOSTAL-RS485.h"
#include "PYLON-CAN.h"
#include "PYLON-LV-CAN.h"
#include "SCHNEIDER-CAN.h"
#include "SMA-BYD-H-CAN.h"
#include "SMA-BYD-HVS-CAN.h"
#include "SMA-LV-CAN.h"
#include "SMA-TRIPOWER-CAN.h"
#include "SOFAR-CAN.h"
#include "SOLAX-CAN.h"
#include "SUNGROW-CAN.h"

// Call to initialize the build-time selected inverter. Safe to call even though inverter was not selected.
void setup_inverter();

#endif
