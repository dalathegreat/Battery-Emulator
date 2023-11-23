#ifndef INVERTERS_H
#define INVERTERS_H

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

#ifdef SOFAR_CAN
#include "SOFAR-CAN.h"
#endif

#ifdef SOLAX_CAN
#include "SOLAX-CAN.h"
#endif

#endif
