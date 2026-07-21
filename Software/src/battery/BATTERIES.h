#ifndef BATTERIES_H
#define BATTERIES_H

#include "Battery.h"
#include "Shunt.h"

// Currently initialized objects for primary/secondary/tertiary battery.
// Null value indicates that battery is not configured/initialized
extern Battery* battery;
extern Battery* battery2;
extern Battery* battery3;

void setup_shunt();

void setup_battery(void);
Battery* create_battery(BatteryType type);

// Returns true if the given battery integration can be instantiated a second
// resp. third time to run batteries in parallel. These are the single source of
// truth for double/triple battery support: setup_battery() gates object creation
// on them, and the web UI uses them to decide whether to offer the checkboxes.
// Keep them in sync with the switch statements in setup_battery().
bool battery_supports_double(BatteryType type);
bool battery_supports_triple(BatteryType type);

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
extern uint16_t user_set_rampdown_SOC;

/* User-selected DALY BMS settings */
extern int user_selected_daly_power_per_percent;
extern int user_selected_daly_power_per_dV;
extern int user_selected_daly_power_per_dV_start;
extern int user_selected_daly_power_per_degree_C;
extern int user_selected_daly_power_at_0_degree_C;

#endif
