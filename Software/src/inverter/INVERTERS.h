#ifndef INVERTERS_H
#define INVERTERS_H

#include "InverterProtocol.h"
extern InverterProtocol* inverter;

// ====================================================================
// FEATURE TOGGLES (Opt-in Compilation)
// Use -D ENABLE_BATT_<NAME> in platformio.ini to include specific batteries
// Use -D ENABLE_ALL_INVERTERS to include everything (Full Version)
// If the MINI_BUILD flag is not declared, automatically enable ENABLE_ALL_INVERTERS!
// ====================================================================
#ifndef MINI_BUILD
  #define ENABLE_ALL_INVERTERS
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_AFORE_CAN)
  #include "AFORE-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_BYD_CAN)
  #include "BYD-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_BYD_MODBUS)
  #include "BYD-MODBUS.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_FERROAMP_CAN)
  #include "FERROAMP-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_FOXESS_CAN)
  #include "FOXESS-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_GROWATT_HV_CAN)
  #include "GROWATT-HV-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_GROWATT_LV_CAN)
  #include "GROWATT-LV-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_GROWATT_WIT_CAN)
  #include "GROWATT-WIT-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_KOSTAL_RS485)
  #include "KOSTAL-RS485.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_PYLON_CAN)
  #include "PYLON-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_PYLON_LV_CAN)
  #include "PYLON-LV-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_PYLON_LV_RS485)
  #include "PYLON-LV-RS485.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SCHNEIDER_CAN)
  #include "SCHNEIDER-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SMA_BYD_H_CAN)
  #include "SMA-BYD-H-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SMA_BYD_HVS_CAN)
  #include "SMA-BYD-HVS-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SMA_LV_CAN)
  #include "SMA-LV-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SOFAR_CAN)
  #include "SOFAR-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SOLARK_LV_CAN)
  #include "SOL-ARK-LV-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SOLAX_CAN)
  #include "SOLAX-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SOLXPOW_CAN)
  #include "SOLXPOW-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_SUNGROW_CAN)
  #include "SUNGROW-CAN.h"
#endif

#if defined(ENABLE_ALL_INVERTERS) || defined(ENABLE_INV_VCU_CAN)
  #include "VCU-CAN.h"
#endif

// ====================================================================

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
extern bool user_selected_inverter_ignore_contactors;
extern bool user_selected_pylon_30koffset;
extern bool user_selected_pylon_invert_byteorder;
extern bool user_selected_inverter_deye_workaround;
extern bool user_selected_primo_gen24;
#endif