#ifndef INVERTERS_H
#define INVERTERS_H

#ifdef SOLAX_CAN
  #include "SOLAX-CAN.h"
#endif

#ifdef CAN_BYD
  #include "BYD-CAN.h"
#endif

#ifdef PYLON_CAN
  #include "PYLON-CAN.h"
#endif

#endif