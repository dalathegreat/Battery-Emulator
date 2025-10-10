#ifndef INVERTERS_H
#define INVERTERS_H

#include "InverterProtocol.h"
extern InverterProtocol* inverter;

#include "AFORE-CAN.h"
#include "BYD-CAN.h"
#include "BYD-MODBUS.h"
#include "FERROAMP-CAN.h"
#include "FOXESS-CAN.h"
#include "GROWATT-HV-CAN.h"
#include "GROWATT-LV-CAN.h"
#include "GROWATT-WIT-CAN.h"
#include "KOSTAL-RS485.h"
#include "PYLON-CAN.h"
#include "PYLON-LV-CAN.h"
#include "SCHNEIDER-CAN.h"
#include "SMA-BYD-H-CAN.h"
#include "SMA-BYD-HVS-CAN.h"
#include "SMA-LV-CAN.h"
#include "SMA-TRIPOWER-CAN.h"
#include "SOFAR-CAN.h"
#include "SOL-ARK-LV-CAN.h"
#include "SOLAX-CAN.h"
#include "SOLXPOW-CAN.h"
#include "SUNGROW-CAN.h"

// Call to initialize the build-time selected inverter. Safe to call even though inverter was not selected.
bool setup_inverter();

extern uint16_t user_selected_pylon_send;
extern uint16_t user_selected_inverter_cells;
extern uint16_t user_selected_inverter_modules;
extern uint16_t user_selected_inverter_cells_per_module;
extern uint16_t user_selected_inverter_voltage_level;
extern uint16_t user_selected_inverter_ah_capacity;
extern uint16_t user_selected_inverter_battery_type;
extern bool user_selected_inverter_ignore_contactors;
extern bool user_selected_pylon_30koffset;
extern bool user_selected_pylon_invert_byteorder;
extern bool user_selected_inverter_deye_workaround;
#endif
