#ifndef BATTERIES_H
#define BATTERIES_H
#include "Shunt.h"

class Battery;

// Currently initialized objects for primary/secondary/tertiary battery.
// Null value indicates that battery is not configured/initialized
extern Battery* battery;
extern Battery* battery2;
extern Battery* battery3;

void setup_shunt();

// ====================================================================
// FEATURE TOGGLES (Opt-in Compilation)
// Use -D ENABLE_BATT_<NAME> in platformio.ini to include specific batteries
// Use -D ENABLE_ALL_BATTERIES to include everything (Full Version)
// If the MINI_BUILD flag is not declared, automatically enable ENABLE_ALL_BATTERIES!
// ====================================================================
#ifndef MINI_BUILD
  #define ENABLE_ALL_BATTERIES
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BMW_I3)
  #include "BMW-I3-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BMW_IX)
  #include "BMW-IX-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BMW_PHEV)
  #include "BMW-PHEV-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BMW_SBOX)
  #include "BMW-SBOX.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BOLT_AMPERA)
  #include "BOLT-AMPERA-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_BYD_ATTO3)
  #include "BYD-ATTO-3-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_CELLPOWER)
  #include "CELLPOWER-BMS.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_CHADEMO)
  #include "CHADEMO-BATTERY.h"
  #include "CHADEMO-CT.h"
  #include "CHADEMO-SHUNTS.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_CMFA_EV)
  #include "CMFA-EV-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_CMP_SMART_CAR)
  #include "CMP-SMART-CAR-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_DALY)
  #include "DALY-BMS.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_ECMP)
  #include "ECMP-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_FORD_MACH_E)
  #include "FORD-MACH-E-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_FOXESS)
  #include "FOXESS-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_GEELY_GEOMETRY_C)
  #include "GEELY-GEOMETRY-C-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_GEELY_SEA)
  #include "GEELY-SEA-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_GROWATT_HV_ARK)
  #include "GROWATT-HV-ARK-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_HYUNDAI_IONIQ_28)
  #include "HYUNDAI-IONIQ-28-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_IMIEV_CZERO)
  #include "IMIEV-CZERO-ION-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_JAGUAR_IPACE)
  #include "JAGUAR-IPACE-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_KIA_64FD)
  #include "KIA-64FD-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_KIA_E_GMP)
  #include "KIA-E-GMP-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_KIA_HYUNDAI_64)
  #include "KIA-HYUNDAI-64-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_KIA_HYUNDAI_HYBRID)
  #include "KIA-HYUNDAI-HYBRID-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_MEB)
  #include "MEB-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_MG_5)
  #include "MG-5-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_MG_HS_PHEV)
  #include "MG-HS-PHEV-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_NISSAN_LEAF)
  #include "NISSAN-LEAF-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_ORION)
  #include "ORION-BMS.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_PYLON)
  #include "PYLON-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RANGE_ROVER_PHEV)
  #include "RANGE-ROVER-PHEV-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RELION_LV)
  #include "RELION-LV-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RENAULT_KANGOO)
  #include "RENAULT-KANGOO-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RENAULT_TWIZY)
  #include "RENAULT-TWIZY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RENAULT_ZOE_GEN1)
  #include "RENAULT-ZOE-GEN1-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RENAULT_ZOE_GEN2)
  #include "RENAULT-ZOE-GEN2-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RIVIAN)
  #include "RIVIAN-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_RJXZS)
  #include "RJXZS-BMS.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_SAMSUNG_SDI_LV)
  #include "SAMSUNG-SDI-LV-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_SANTA_FE_PHEV)
  #include "SANTA-FE-PHEV-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_SIMPBMS)
  #include "SIMPBMS-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_SONO)
  #include "SONO-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_TESLA)
  #include "TESLA-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_TESLA_LEGACY)
  #include "TESLA-LEGACY-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_TEST_FAKE)
  #include "TEST-FAKE-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_THINK)
  #include "THINK-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_THUNDERSTRUCK)
  #include "THUNDERSTRUCK-BMS.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_VOLVO_SPA)
  #include "VOLVO-SPA-BATTERY.h"
#endif

#if defined(ENABLE_ALL_BATTERIES) || defined(ENABLE_BATT_VOLVO_SPA_HYBRID)
  #include "VOLVO-SPA-HYBRID-BATTERY.h"
#endif

// ====================================================================

void setup_battery(void);
Battery* create_battery(BatteryType type);

extern uint16_t user_selected_max_pack_voltage_dV;
extern uint16_t user_selected_min_pack_voltage_dV;
extern uint16_t user_selected_max_cell_voltage_mV;
extern uint16_t user_selected_min_cell_voltage_mV;
extern bool user_selected_use_estimated_SOC;
extern bool user_selected_LEAF_interlock_mandatory;
extern bool user_selected_tesla_digital_HVIL;
extern uint16_t user_selected_tesla_GTW_country;
extern bool user_selected_tesla_GTW_rightHandDrive;
extern uint16_t user_selected_tesla_GTW_mapRegion;
extern uint16_t user_selected_tesla_GTW_chassisType;
extern uint16_t user_selected_tesla_GTW_packEnergy;
extern uint16_t user_selected_pylon_baudrate;

#endif