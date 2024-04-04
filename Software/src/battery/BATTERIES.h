#ifndef BATTERIES_H
#define BATTERIES_H

#include "../../USER_SETTINGS.h"

#ifdef BMW_I3_BATTERY
#include "BMW-I3-BATTERY.h"  //See this file for more i3 battery settings
#endif

#ifdef CHADEMO_BATTERY
#include "CHADEMO-BATTERY.h"  //See this file for more Chademo settings
#endif

#ifdef IMIEV_CZERO_ION_BATTERY
#include "IMIEV-CZERO-ION-BATTERY.h"  //See this file for more triplet battery settings
#endif

#ifdef KIA_E_GMP_BATTERY
#include "KIA-E-GMP-BATTERY.h"  //See this file for more GMP battery settings
#endif

#ifdef KIA_HYUNDAI_64_BATTERY
#include "KIA-HYUNDAI-64-BATTERY.h"  //See this file for more 64kWh battery settings
#endif

#ifdef NISSAN_LEAF_BATTERY
#include "NISSAN-LEAF-BATTERY.h"  //See this file for more LEAF battery settings
#endif

#ifdef RENAULT_KANGOO_BATTERY
#include "RENAULT-KANGOO-BATTERY.h"  //See this file for more Kangoo battery settings
#endif

#ifdef RENAULT_ZOE_BATTERY
#include "RENAULT-ZOE-BATTERY.h"  //See this file for more Zoe battery settings
#endif

#ifdef SANTA_FE_PHEV_BATTERY
#include "SANTA-FE-PHEV-BATTERY.h"  //See this file for more Santa Fe PHEV battery settings
#endif

#ifdef TESLA_MODEL_3_BATTERY
#include "TESLA-MODEL-3-BATTERY.h"  //See this file for more Tesla battery settings
#endif

#ifdef TEST_FAKE_BATTERY
#include "TEST-FAKE-BATTERY.h"  //See this file for more Fake battery settings
#endif

#ifdef VOLVO_SPA_BATTERY
#include "VOLVO-SPA-BATTERY.h"  //See this file for more XC40 Recharge/Polestar2 settings
#endif

#ifdef SERIAL_LINK_RECEIVER
#include "SERIAL-LINK-RECEIVER-FROM-BATTERY.h"  //See this file for more Serial-battery settings
#endif

#ifdef SERIAL_LINK_RECEIVER  // The serial thing does its thing
void receive_can_battery();
#else
void receive_can_battery(CAN_frame_t rx_frame);
#endif
#ifdef CAN_FD
void receive_canfd_battery(CANFDMessage frame);
#endif
void update_values_battery();
void send_can_battery();
void setup_battery(void);

#endif
