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

/* Nissan Leaf charge taper. The LBC reports a near-binary charge limit, which oscillates on imbalanced packs.
   Tapering charge power as the highest cell nears a CV setpoint keeps charging smooth instead of bursty. */
extern bool user_set_leaf_taper_active;
extern uint16_t user_set_leaf_taper_target_mV;
extern uint16_t user_set_leaf_taper_band_mV;

/* User-selected DALY BMS settings */
extern int user_selected_daly_power_per_percent;
extern int user_selected_daly_power_per_dV;
extern int user_selected_daly_power_per_dV_start;
extern int user_selected_daly_power_per_degree_C;
extern int user_selected_daly_power_at_0_degree_C;

#endif
