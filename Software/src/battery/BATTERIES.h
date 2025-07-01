#ifndef BATTERIES_H
#define BATTERIES_H
#include "../../USER_SETTINGS.h"

class Battery;

// Currently initialized objects for primary and secondary battery.
// Null value indicates that battery is not configured/initialized
extern Battery* battery;
extern Battery* battery2;

void setup_can_shunt();

#include "BMW-I3-BATTERY.h"
#include "BMW-IX-BATTERY.h"
#include "BMW-PHEV-BATTERY.h"
#include "BMW-SBOX.h"
#include "BOLT-AMPERA-BATTERY.h"
#include "BYD-ATTO-3-BATTERY.h"
#include "CELLPOWER-BMS.h"

#include "CHADEMO-BATTERY.h"
#include "CHADEMO-SHUNTS.h"

#include "CMFA-EV-BATTERY.h"
#include "DALY-BMS.h"
#include "ECMP-BATTERY.h"
#include "FOXESS-BATTERY.h"
#include "GEELY-GEOMETRY-C-BATTERY.h"
#include "IMIEV-CZERO-ION-BATTERY.h"
#include "JAGUAR-IPACE-BATTERY.h"
#include "KIA-E-GMP-BATTERY.h"
#include "KIA-HYUNDAI-64-BATTERY.h"
#include "KIA-HYUNDAI-HYBRID-BATTERY.h"
#include "MEB-BATTERY.h"
#include "MG-5-BATTERY.h"
#include "NISSAN-LEAF-BATTERY.h"
#include "ORION-BMS.h"
#include "PYLON-BATTERY.h"
#include "RANGE-ROVER-PHEV-BATTERY.h"
#include "RENAULT-KANGOO-BATTERY.h"
#include "RENAULT-TWIZY.h"
#include "RENAULT-ZOE-GEN1-BATTERY.h"
#include "RENAULT-ZOE-GEN2-BATTERY.h"
#include "RJXZS-BMS.h"
#include "SANTA-FE-PHEV-BATTERY.h"
#include "SIMPBMS-BATTERY.h"
#include "SONO-BATTERY.h"
#include "TESLA-BATTERY.h"
#include "TEST-FAKE-BATTERY.h"
#include "VOLVO-SPA-BATTERY.h"
#include "VOLVO-SPA-HYBRID-BATTERY.h"

void setup_battery(void);

#endif
