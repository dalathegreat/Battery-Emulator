#ifndef INVERTERS_H
#define INVERTERS_H

#ifdef SOLAX_CAN
  #include "SOLAX-CAN.h"
#endif

#ifdef CAN_BYD
  #include "BYD-CAN.h"
#endif

#ifdef SMA_CAN
  #include "SMA-CAN.h"
#endif

#ifdef PYLON_CAN
  #include "PYLON-CAN.h"
#endif

#ifdef MODBUS_BYD
  #include "MODBUS-BYD.h"
#endif

#ifdef MODBUS_LUNA2000
  #include "MODBUS-LUNA2000.h"
#endif

#endif