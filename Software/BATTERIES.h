#ifndef BATTERIES_H
#define BATTERIES_H

#ifdef BATTERY_TYPE_LEAF
  #include "NISSAN-LEAF-BATTERY.h" //See this file for more LEAF battery settings
#endif

#ifdef TESLA_MODEL_3_BATTERY
  #include "TESLA-MODEL-3-BATTERY.h" //See this file for more Tesla battery settings
#endif

#ifdef RENAULT_ZOE_BATTERY
  #include "RENAULT-ZOE-BATTERY.h" //See this file for more Zoe battery settings
#endif

#endif