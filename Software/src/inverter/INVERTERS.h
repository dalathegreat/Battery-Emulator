#ifndef INVERTERS_H
#define INVERTERS_H

#include "InverterProtocol.h"

#include <stdint.h>

// Inverter contactor workaround modes
enum inverter_contactor_mode_enum {
  NoWorkaround = 0,        // Normal operation - follow inverter's open/close requests
  AlwaysClosed = 1,        // Keep contactors always closed
  LockAfterFirstClose = 2  // Wait for first close request, then ignore opens
};
extern InverterProtocol* inverter;

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
extern uint16_t user_selected_inverter_foxess_type;
extern uint16_t user_selected_inverter_foxess_subtype;
extern uint16_t user_selected_inverter_foxess_modules;
extern inverter_contactor_mode_enum user_selected_inverter_contactor_mode;
extern bool user_selected_pylon_30koffset;
extern bool user_selected_pylon_invert_byteorder;
extern bool user_selected_inverter_deye_workaround;
extern bool user_selected_inverter_long_CAN_timeout;
extern bool user_selected_primo_gen24;
extern bool inverter_low_pass_filter;
#endif
