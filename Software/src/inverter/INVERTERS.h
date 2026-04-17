#ifndef INVERTERS_H
#define INVERTERS_H

#include "InverterProtocol.h"

// Inverter contactor workaround modes
enum inverter_contactor_mode_enum {
  NoWorkaround = 0,        // Normal operation - follow inverter's open/close requests
  AlwaysClosed = 1,        // Keep contactors always closed
  LockAfterFirstClose = 2  // Wait for first close request, then ignore opens
};
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
#include "PYLON-LV-RS485.h"
#include "SCHNEIDER-CAN.h"
#include "SMA-BYD-H-CAN.h"
#include "SMA-BYD-HVS-CAN.h"
#include "SMA-LV-CAN.h"
#include "SOFAR-CAN.h"
#include "SOL-ARK-LV-CAN.h"
#include "SOLAX-CAN.h"
#include "SOLXPOW-CAN.h"
#include "SUNGROW-CAN.h"
#include "VCU-CAN.h"

// Call to initialize the build-time selected inverter. Safe to call even though inverter was not selected.
bool setup_inverter();

extern uint16_t user_selected_pylon_send;
extern uint16_t user_selected_inverter_cells;
extern uint16_t user_selected_inverter_modules;
extern uint16_t user_selected_inverter_cells_per_module;
extern uint16_t user_selected_inverter_voltage_level;
extern uint16_t user_selected_inverter_ah_capacity;
extern uint16_t user_selected_inverter_battery_type;
extern uint16_t user_selected_inverter_sungrow_type;
extern uint16_t user_selected_inverter_pylon_type;
extern inverter_contactor_mode_enum user_selected_inverter_contactor_mode;
extern bool user_selected_pylon_30koffset;
extern bool user_selected_pylon_invert_byteorder;
extern bool user_selected_inverter_deye_workaround;
extern bool user_selected_primo_gen24;
#endif
