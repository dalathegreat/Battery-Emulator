#ifndef BATTERIES_H
#define BATTERIES_H
#include "../../USER_SETTINGS.h"

#ifdef BMW_SBOX
#include "BMW-SBOX.h"
void handle_incoming_can_frame_shunt(CAN_frame rx_frame);
void transmit_can_shunt();
void setup_can_shunt();
#endif

#ifdef BMW_I3_BATTERY
#include "BMW-I3-BATTERY.h"
#endif

#ifdef BMW_IX_BATTERY
#include "BMW-IX-BATTERY.h"
#endif

#ifdef BMW_PHEV_BATTERY
#include "BMW-PHEV-BATTERY.h"
#endif

#ifdef BOLT_AMPERA_BATTERY
#include "BOLT-AMPERA-BATTERY.h"
#endif

#ifdef BYD_ATTO_3_BATTERY
#include "BYD-ATTO-3-BATTERY.h"
#endif

#ifdef CELLPOWER_BMS
#include "CELLPOWER-BMS.h"
#endif

#ifdef CHADEMO_BATTERY
#include "CHADEMO-BATTERY.h"
#include "CHADEMO-SHUNTS.h"
#endif

#ifdef FOXESS_BATTERY
#include "FOXESS-BATTERY.h"
#endif

#ifdef ORION_BMS
#include "ORION-BMS.h"
#endif

#ifdef SONO_BATTERY
#include "SONO-BATTERY.h"
#endif

#ifdef STELLANTIS_ECMP_BATTERY
#include "ECMP-BATTERY.h"
#endif

#ifdef IMIEV_CZERO_ION_BATTERY
#include "IMIEV-CZERO-ION-BATTERY.h"
#endif

#ifdef JAGUAR_IPACE_BATTERY
#include "JAGUAR-IPACE-BATTERY.h"
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

#ifdef MEB_BATTERY
#include "MEB-BATTERY.h"
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

#ifdef DALY_BMS
#include "DALY-BMS.h"
#endif

#ifdef RJXZS_BMS
#include "RJXZS-BMS.h"
#endif

#ifdef RANGE_ROVER_PHEV_BATTERY
#include "RANGE-ROVER-PHEV-BATTERY.h"
#endif

#ifdef RENAULT_KANGOO_BATTERY
#include "RENAULT-KANGOO-BATTERY.h"
#endif

#ifdef RENAULT_TWIZY_BATTERY
#include "RENAULT-TWIZY.h"
#endif

#ifdef RENAULT_ZOE_GEN1_BATTERY
#include "RENAULT-ZOE-GEN1-BATTERY.h"
#endif

#ifdef RENAULT_ZOE_GEN2_BATTERY
#include "RENAULT-ZOE-GEN2-BATTERY.h"
#endif

#ifdef SANTA_FE_PHEV_BATTERY
#include "SANTA-FE-PHEV-BATTERY.h"
#endif

#ifdef SIMPBMS_BATTERY
#include "SIMPBMS-BATTERY.h"
#endif

#if defined(TESLA_MODEL_SX_BATTERY) || defined(TESLA_MODEL_3Y_BATTERY)
#define TESLA_BATTERY
#include "TESLA-BATTERY.h"
#endif

#ifdef TEST_FAKE_BATTERY
#include "TEST-FAKE-BATTERY.h"
#endif

#ifdef VOLVO_SPA_BATTERY
#include "VOLVO-SPA-BATTERY.h"
#endif

#ifdef VOLVO_SPA_HYBRID_BATTERY
#include "VOLVO-SPA-HYBRID-BATTERY.h"
#endif

void setup_battery(void);
void update_values_battery();

#ifdef RS485_BATTERY_SELECTED
void transmit_rs485();
void receive_RS485();
#else
void handle_incoming_can_frame_battery(CAN_frame rx_frame);
void transmit_can_battery();
#endif

#ifdef DOUBLE_BATTERY
void update_values_battery2();
void handle_incoming_can_frame_battery2(CAN_frame rx_frame);
#endif

#endif
