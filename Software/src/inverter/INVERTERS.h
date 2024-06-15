#ifndef INVERTERS_H
#define INVERTERS_H

#include "../../USER_SETTINGS.h"
#define INVERTER_CAN_BUFFER_SIZE 0

#ifdef BYD_CAN
#include "BYD-CAN.h"
#endif

#ifdef BYD_MODBUS
#include "BYD-MODBUS.h"
#endif

#ifdef LUNA2000_MODBUS
#include "LUNA2000-MODBUS.h"
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
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"  // This include is annoying, consider defining a frame type in types.h
void update_values_can_inverter();
void receive_can_inverter(CAN_frame_t rx_frame);
void send_can_inverter();
#endif

#ifdef MODBUS_INVERTER_SELECTED
void update_modbus_registers_inverter();
#endif

#endif
