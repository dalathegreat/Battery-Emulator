#ifndef INVERTERS_H
#define INVERTERS_H

#include "../../USER_SETTINGS.h"

#ifdef BYD_CAN
#include "BYD-CAN.h"
#endif

#ifdef BYD_MODBUS
#include "BYD-MODBUS.h"
#endif

#ifdef BYD_SMA
#include "BYD-SMA.h"
#endif

#ifdef BYD_KOSTAL_RS485
#include "KOSTAL-RS485.h"
#endif

#ifdef FOXESS_CAN
#include "FOXESS-CAN.h"
#endif

#ifdef PYLON_CAN
#include "PYLON-CAN.h"
#endif

#ifdef SMA_CAN
#include "SMA-CAN.h"
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

#ifdef SERIAL_LINK_TRANSMITTER
#include "SERIAL-LINK-TRANSMITTER-INVERTER.h"
#endif

#ifdef CAN_INVERTER_SELECTED
void update_values_can_inverter();
void receive_can_inverter(CAN_frame rx_frame);
void send_can_inverter();
#endif

#ifdef MODBUS_INVERTER_SELECTED
void update_modbus_registers_inverter();
#endif

#ifdef RS485_INVERTER_SELECTED
void receive_RS485();
void update_RS485_registers_inverter();
#endif

#endif
