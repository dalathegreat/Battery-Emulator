#ifndef INVERTERS_H
#define INVERTERS_H

#include "../../USER_SETTINGS.h"

// Selection of inverters to the common image.
#ifdef BUILD_EM_ALL
#define AFORE_CAN
#define BYD_CAN_DEYE
#define BYD_CAN
#define BYD_MODBUS
#define BYD_KOSTAL_RS485
#define FERROAMP_CAN
#define FOXESS_CAN
#define GROWATT_HV_CAN
#define GROWATT_LV_CAN
#define PYLON_CAN
#define PYLON_LV_CAN
#define SCHNEIDER_CAN
#define SMA_BYD_H_CAN
#define SMA_BYD_HVS_CAN
#define SMA_LV_CAN
#define SMA_TRIPOWER_CAN
#define SOFAR_CAN
#define SOLAX_CAN
#define SUNGROW_CAN
#endif

#ifdef AFORE_CAN
#include "AFORE-CAN.h"
#endif

#ifdef BYD_CAN_DEYE
#define BYD_CAN
#endif

#ifdef BYD_CAN
#include "BYD-CAN.h"
#endif

#ifdef BYD_MODBUS
#include "BYD-MODBUS.h"
#endif

#ifdef BYD_KOSTAL_RS485
#include "KOSTAL-RS485.h"
#endif

#ifdef FERROAMP_CAN
#include "FERROAMP-CAN.h"
#endif

#ifdef FOXESS_CAN
#include "FOXESS-CAN.h"
#endif

#ifdef GROWATT_HV_CAN
#include "GROWATT-HV-CAN.h"
#endif

#ifdef GROWATT_LV_CAN
#include "GROWATT-LV-CAN.h"
#endif

#ifdef PYLON_CAN
#include "PYLON-CAN.h"
#endif

#ifdef PYLON_LV_CAN
#include "PYLON-LV-CAN.h"
#endif

#ifdef SCHNEIDER_CAN
#include "SCHNEIDER-CAN.h"
#endif

#ifdef SMA_BYD_H_CAN
#include "SMA-BYD-H-CAN.h"
#endif

#ifdef SMA_BYD_HVS_CAN
#include "SMA-BYD-HVS-CAN.h"
#endif

#ifdef SMA_LV_CAN
#include "SMA-LV-CAN.h"
#endif

#ifdef SMA_TRIPOWER_CAN
#include "SMA-TRIPOWER-CAN.h"
#endif

#ifdef SOFAR_CAN
#include "SOFAR-CAN.h"
#endif

#ifdef SOLAX_CAN
#include "SOLAX-CAN.h"
#endif

#ifdef SUNGROW_CAN
#include "SUNGROW-CAN.h"
#endif

#ifdef CAN_INVERTER_SELECTED
//void update_values_can_inverter();
//void map_can_frame_to_variable_inverter(CAN_frame rx_frame);
//void transmit_can_inverter();
#endif

#ifdef MODBUS_INVERTER_SELECTED
//void update_modbus_registers_inverter();
#endif

#ifdef RS485_INVERTER_SELECTED
//void receive_RS485();
//void update_RS485_registers_inverter();
//void setup_inverter();
#endif

#endif
