#ifndef BATTERIES_H
#define BATTERIES_H

#ifdef BMW_I3_BATTERY
#include "BMW-I3-BATTERY.h"  //See this file for more i3 battery settings
#endif

#ifdef CHADEMO_BATTERY
#include "CHADEMO-BATTERY.h"  //See this file for more Chademo settings
#endif

#ifdef IMIEV_CZERO_ION_BATTERY
#include "IMIEV-CZERO-ION-BATTERY.h"  //See this file for more triplet battery settings
#endif

#ifdef KIA_HYUNDAI_64_BATTERY
#include "KIA-HYUNDAI-64-BATTERY.h"  //See this file for more 64kWh battery settings
#endif

#ifdef NISSAN_LEAF_BATTERY
#include "NISSAN-LEAF-BATTERY.h"  //See this file for more LEAF battery settings
#endif

#ifdef RENAULT_ZOE_BATTERY
#include "RENAULT-ZOE-BATTERY.h"  //See this file for more Zoe battery settings
#endif

#ifdef TESLA_MODEL_3_BATTERY
#include "TESLA-MODEL-3-BATTERY.h"  //See this file for more Tesla battery settings
#endif

#ifdef TEST_FAKE_BATTERY
#include "TEST-FAKE-BATTERY.h"  //See this file for more Fake battery settings
#endif

#ifdef SERIAL_LINK_RECEIVER_FROM_BATTERY
#include "SERIAL-LINK-RECEIVER-FROM-BATTERY.h"
#endif

#endif
