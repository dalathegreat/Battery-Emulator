#ifndef BATTERIES_H
#define BATTERIES_H

#include "../shunt/Shunt.h"
#include "Battery.h"

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

// Returns true if the integration writes datalayer.battery.info.total_capacity_Wh
// itself, either from the BMS or from a model specific constant. For those the
// stored "Battery capacity" setting is overwritten by the driver, so the web UI
// hides the row instead of offering an edit that does not stick.
bool battery_detects_capacity(BatteryType type);

bool byd_cell_balance_times_available(uint8_t index);
bool request_byd_cell_balance_times(uint8_t index);
String byd_cell_balance_times_json(uint8_t index);

extern uint16_t user_selected_max_pack_voltage_dV;
extern uint16_t user_selected_min_pack_voltage_dV;
extern uint16_t user_selected_max_cell_voltage_mV;
extern uint16_t user_selected_min_cell_voltage_mV;
extern bool user_selected_use_estimated_SOC;
extern bool user_selected_use_estimated_charge_limits;
extern bool user_selected_LEAF_interlock_mandatory;
extern uint8_t user_selected_LEAF_chg_sta_rq;
extern bool user_selected_LEAF_auto_current_offset;
extern bool user_selected_tesla_digital_HVIL;
extern uint16_t user_selected_tesla_GTW_country;
extern bool user_selected_tesla_GTW_rightHandDrive;
extern uint16_t user_selected_tesla_GTW_mapRegion;
extern uint16_t user_selected_tesla_GTW_chassisType;
extern uint16_t user_selected_tesla_GTW_packEnergy;
extern uint16_t user_selected_pylon_baudrate;

/* User-selected DALY BMS settings */
extern int user_selected_daly_power_per_percent;
extern int user_selected_daly_power_per_dV;
extern int user_selected_daly_power_per_dV_start;
extern int user_selected_daly_power_per_degree_C;
extern int user_selected_daly_power_at_0_degree_C;

// --- JK Active Balancer ---
/* User-selected JK Active Balancer settings. Percent values are stored x10 (dpct),
   currents x1000 (mA); the driver converts them back in setup(). */
extern uint32_t user_selected_jk_max_charge_W;
extern uint32_t user_selected_jk_max_discharge_W;
extern uint16_t user_selected_jk_soc_max_dpct;
extern uint16_t user_selected_jk_soc_min_dpct;
extern uint16_t user_selected_jk_deadband_mA;
extern uint32_t user_selected_jk_recal_rest_ms;
extern uint16_t user_selected_jk_recal_rest_mA;
extern uint16_t user_selected_jk_recal_soc_bottom_dpct;
extern uint16_t user_selected_jk_recal_soc_top_dpct;
extern uint16_t user_selected_jk_dis_ramp_dpct;
extern uint16_t user_selected_jk_dis_ramp_bottom_dpct;
extern uint32_t user_selected_jk_dis_min_W;
extern uint16_t user_selected_jk_dis_cutoff_hyst_dV;
extern uint16_t user_selected_jk_chg_ramp_dpct;
extern uint32_t user_selected_jk_chg_min_W;
extern uint8_t user_selected_jk_max_balancers;
extern bool user_selected_jk_reverse_current;
extern uint16_t user_selected_jk_cab500_can_id;
extern uint16_t user_selected_jk2_cab500_can_id;
extern bool user_selected_jk_lfp;
extern uint16_t user_selected_jk_cell_max_mV;
extern uint16_t user_selected_jk_cell_min_mV;
extern uint16_t user_selected_jk_cell_dev_mV;
extern uint16_t user_selected_jk_pack_max_mVpc;
extern uint16_t user_selected_jk_pack_min_mVpc;
extern uint16_t user_selected_jk_vfull_on_mV;
extern uint16_t user_selected_jk_vfull_off_mV;
extern uint16_t user_selected_jk_vempty_on_mV;
extern uint16_t user_selected_jk_vempty_off_mV;
extern uint8_t user_selected_jk_cells;
extern uint8_t user_selected_jk_cells_per_balancer;
extern bool user_selected_jk_low_voltage;
extern bool user_selected_jk_bridge;
extern uint8_t user_selected_jk_fw_version;
extern uint16_t user_selected_jk_fw_mask;

#endif
