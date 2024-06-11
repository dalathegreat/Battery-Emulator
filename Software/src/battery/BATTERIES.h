#ifndef BATTERIES_H
#define BATTERIES_H

#include "../../USER_SETTINGS.h"

#ifdef BMW_I3_BATTERY
#include "BMW-I3-BATTERY.h"
#endif

#ifdef BYD_ATTO_3_BATTERY
#include "BYD-ATTO-3-BATTERY.h"
#endif

#ifdef CHADEMO_BATTERY
#include "CHADEMO-BATTERY.h"
#endif

#ifdef IMIEV_CZERO_ION_BATTERY
#include "IMIEV-CZERO-ION-BATTERY.h"
#endif

#ifdef KIA_E_GMP_BATTERY
#include "KIA-E-GMP-BATTERY.h"
#endif

#ifdef KIA_HYUNDAI_64_BATTERY
#include "KIA-HYUNDAI-64-BATTERY.h"
#endif

#ifdef KIA_HYUNDAI_HYBRID_BATTERY
#include "KIA-HYUNDAI-HYBRID-BATTERY.h"
#endif

#ifdef MG_5_BATTERY
#include "MG-5-BATTERY.h"
#endif

#ifdef NISSAN_LEAF_BATTERY
#include "NISSAN-LEAF-BATTERY.h"
#endif

#ifdef PYLON_BATTERY
#include "PYLON-BATTERY.h"
#endif

#ifdef RENAULT_KANGOO_BATTERY
#include "RENAULT-KANGOO-BATTERY.h"
#endif

#ifdef RENAULT_ZOE_BATTERY
#include "RENAULT-ZOE-BATTERY.h"
#endif

#ifdef SANTA_FE_PHEV_BATTERY
#include "SANTA-FE-PHEV-BATTERY.h"
#endif

#ifdef TESLA_MODEL_3_BATTERY
#include "TESLA-MODEL-3-BATTERY.h"
#endif

#ifdef TEST_FAKE_BATTERY
#include "TEST-FAKE-BATTERY.h"
#endif

#ifdef VOLVO_SPA_BATTERY
#include "VOLVO-SPA-BATTERY.h"
#endif

#ifdef SERIAL_LINK_RECEIVER
#include "SERIAL-LINK-RECEIVER-FROM-BATTERY.h"
#endif

#ifdef SERIAL_LINK_RECEIVER  // The serial thing does its thing
void receive_can_battery();
#else
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"  // This include is annoying, consider defining a frame type in types.h
void receive_can_battery(CAN_frame_t rx_frame);
#endif
#ifdef CAN_FD
void receive_canfd_battery(CANFDMessage frame);
#endif

void update_values_battery();
void send_can_battery();
void setup_battery(void);

#endif
