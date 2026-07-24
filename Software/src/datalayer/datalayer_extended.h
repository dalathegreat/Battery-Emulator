#ifndef _DATALAYER_EXTENDED_H_
#define _DATALAYER_EXTENDED_H_

#include <stdint.h>
#include <string.h>

struct DATALAYER_INFO_BOLTAMPERA {
  /** uint16_t */
  /** PID polling parameters */
  uint16_t battery_5V_ref;
  uint16_t battery_capacity_my17_18;
  uint16_t battery_capacity_my19plus;
  uint16_t battery_SOC_display;
  uint16_t battery_SOC_raw_highprec;
  uint16_t battery_max_temperature;
  uint16_t battery_min_temperature;
  uint16_t battery_max_cell_voltage;
  uint16_t battery_min_cell_voltage;
  uint16_t battery_lowest_cell;
  uint16_t battery_highest_cell;
  uint16_t battery_internal_resistance;
  uint16_t battery_voltage_polled;
  uint16_t battery_vehicle_isolation;
  uint16_t battery_isolation_kohm;
  uint16_t battery_HV_locked;
  uint16_t battery_crash_event;
  uint16_t battery_HVIL;
  uint16_t battery_HVIL_status;
  uint16_t battery_cell_average_voltage;
  uint16_t battery_cell_average_voltage_2;
  uint16_t battery_terminal_voltage;
  uint16_t battery_ignition_power_mode;

  int16_t battery_module_temp_1;
  int16_t battery_module_temp_2;
  int16_t battery_module_temp_3;
  int16_t battery_module_temp_4;
  int16_t battery_module_temp_5;
  int16_t battery_module_temp_6;
  int16_t battery_current_7E7;
  int16_t battery_current_7E4;
};

struct DATALAYER_INFO_BMWPHEV {
  uint64_t min_cell_voltage_data_age;
  uint64_t max_cell_voltage_data_age;

  /** uint16_t */
  /** Min/Max Cell SOH*/
  uint16_t min_soh_state;
  uint16_t max_soh_state;
  uint16_t iso_safety_int_kohm;  //STAT_ISOWIDERSTAND_INT_WERT
  uint16_t iso_safety_ext_kohm;  //STAT_ISOWIDERSTAND_EXT_STD_WERT
  uint16_t iso_safety_trg_kohm;
  uint16_t iso_safety_kohm;  //STAT_R_ISO_ROH_01_WERT

  int16_t battery_voltage_after_contactor;
  int16_t allowable_charge_amps;
  int16_t allowable_discharge_amps;

  /** uint8_t */
  /** Status isolation external, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_iso_ext;
  /** uint8_t */
  /** Status isolation external, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_iso_int;
  /** uint8_t */
  /** Status cooling valve error, 0 not evaluated, 1 OK valve closed, 2 error active valve open, 3 Invalid signal*/
  uint8_t ST_valve_cooling;
  /** uint8_t */
  /** Status interlock error, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_interlock;
  /** uint8_t */
  /** Status precharge, 0 no statement, 1 Not active closing not blocked, 2 error precharge blocked, 3 Invalid signal*/
  uint8_t ST_precharge;
  /** uint8_t */
  /** Status DC switch, 0 contactors open, 1 precharge ongoing, 2 contactors engaged, 3 Invalid signal*/
  uint8_t ST_DCSW;
  /** uint8_t */
  /** Status emergency, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_EMG;
  /** uint8_t */
  /** Status welding detection, 0 Contactors OK, 1 One contactor welded, 2 Two contactors welded, 3 Invalid signal*/
  uint8_t ST_WELD;
  /** uint8_t */
  /** Status isolation, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_isolation;
  /** uint8_t */
  /** Status cold shutoff valve, 0 OK, 1 Short circuit to GND, 2 Short circuit to 12V, 3 Line break, 6 Driver error, 12 Stuck, 13 Stuck, 15 Invalid Signal*/
  uint8_t ST_cold_shutoff_valve;
  /** Status HVIL, 1 HVIL OK, 0 HVIL disconnected*/
  //uint8_t hvil_status;
  uint8_t battery_request_open_contactors;
  uint8_t battery_request_open_contactors_instantly;
  uint8_t battery_request_open_contactors_fast;
  uint8_t battery_charging_condition_delta;
  uint8_t dtc_count;                 // Number of DTCs present
  uint8_t iso_safety_ext_plausible;  //STAT_ISOWIDERSTAND_EXT_TRG_PLAUS
  uint8_t iso_safety_int_plausible;
  uint8_t iso_safety_trg_plausible;
  uint8_t iso_safety_kohm_quality;  //STAT_R_ISO_ROH_QAL_01_INFO Quality of measurement 0-21 (higher better)
  uint8_t balancing_status;

  bool dtc_read_failed;          // Indicates last read attempt failed
  bool UserRequestDTCreset;      /** User requesting DTC reset via WebUI*/
  bool UserRequestBMSReset;      /** User requesting BMS reset via WebUI*/
  bool UserRequestIsolationTest; /** User requesting isolation test via WebUI*/
};

struct DATALAYER_INFO_BMWIX {
  uint32_t
      dtc_codes[32];  // Array of DTC codes (3 bytes each, stored as uint32) (Same array used on other BMW integrations)
  uint8_t dtc_status[32];              // Status byte for each DTC (Same array used on other BMW integrations)
  unsigned long dtc_last_read_millis;  // Timestamp of last successful read
  uint8_t dtc_count;                   // Number of DTCs present
  bool dtc_read_in_progress;           // Flag to prevent concurrent reads
  bool dtc_read_failed;                // Indicates last read attempt failed
  bool UserRequestDTCreset;            /** User requesting DTC reset via WebUI*/
  bool UserRequestBMSReset;            /** User requesting BMS reset via WebUI*/
};

struct DATALAYER_INFO_BYDATTO3 {
  /** SOC% estimate. Estimated from total pack voltage */
  uint16_t SOC_estimated;
  /** SOC% raw battery value. Highprecision. Can be locked if pack is crashed */
  uint16_t SOC_highprec;
  /** SOC% polled OBD2 value. Can be locked if pack is crashed */
  uint16_t SOC_polled;
  /** Pack voltage from 0x438, deci-volts. Zero until the frame is received */
  uint16_t pack_voltage_dV;
  /** Insulation resistance from 0x43A, Ohm per volt. Multiply by pack voltage for Ohms. Zero is a valid fault reading */
  uint16_t insulation_ohm_per_volt;
  /** True once a checksum-valid 0x43A has been seen */
  bool insulation_valid;
  uint16_t chargePower;
  uint16_t charge_times;
  uint16_t dischargePower;
  uint16_t total_charged_ah;
  uint16_t total_discharged_ah;
  uint16_t total_charged_kwh;
  uint16_t total_discharged_kwh;
  uint16_t times_full_power;
  uint16_t BMS_capacity_original_calibration;
  uint16_t BMC_SOC_original_calibration;
  uint16_t BMS_capacity_current_calibration;
  uint16_t BMC_SOC_current_calibration;
  uint16_t seed;
  uint16_t solvedKey;
  uint16_t calibrationTargetSOC;
  uint16_t calibrationTargetAH;

  /** int16_t */
  /** All the temperature sensors inside the battery pack*/
  int16_t battery_temperatures[13];

  uint8_t discharge_status;
  uint8_t BMS_min_cell_voltage_number;
  uint8_t BMS_min_temp_module_number;
  uint8_t BMS_max_cell_voltage_number;
  uint8_t BMS_max_temp_module_number;
  uint8_t servicemode;
  /** bool */
  /** User requesting crash reset via WebUI*/
  bool UserRequestCrashReset;
  /** bool */
  /** User requesting SOC calibration via WebUI*/
  bool UserRequestCalibrateSOC;
  /** uint8_t */
  /** Software contactor control state, mirrors battery class state machine */
  uint8_t contactor_control_state;
  /** uint8_t */
  /** Raw contactor state feedback reported by BMS in 0x344 byte 0 */
  uint8_t contactor_feedback;
  /** Decoded from contactor_feedback bit7 */
  bool contactor_main_closed;
  /** Decoded from contactor_feedback bit6 */
  bool contactor_precharging;
  /** Decoded from contactor_feedback bit2 */
  bool contactor_hv_active;
  /** Decoded from contactor_feedback bit1, set in idle/drive states, clear while charging */
  bool contactor_drive_flag;
  /** Decoded from contactor_feedback bit0, set while AC charging */
  bool contactor_charge_flag;
  /** bool */
  /** Enable automatic SOC calibration to 100% when physically full (taper + low current) */
  bool auto_calibrate_soc_enabled;
  /** uint8_t */
  /** Drift threshold (%) before auto-calibrate triggers */
  uint8_t auto_calibrate_soc_drift_percent;

  // Auto-calibration live status
  uint32_t autocal_dwell_accumulated_ms;
  uint32_t autocal_grace_timer_ms;
  float autocal_drift_percent;
  int16_t autocal_current_dA;
  bool autocal_crit_taper;
  bool autocal_crit_low_current;
  bool autocal_crit_dwell;
  bool autocal_crit_drift;
  bool autocal_crit_cooldown_ready;
  bool autocal_crit_contactors;

  // DTC readout (UDS 0x19 0x02). Codes packed as raw 3 bytes in a uint32, rendered to string in HTML.
  bool dtc_read_in_progress;
  bool UserRequestDTCreadout;  // User requesting DTC readout via WebUI
  bool UserRequestDTCreset;    // User requesting DTC erase via WebUI
};

struct DATALAYER_INFO_CELLPOWER {
  /** bool */
  /** All values either True or false */
  bool system_state_discharge;
  bool system_state_charge;
  bool system_state_cellbalancing;
  bool system_state_tricklecharge;
  bool system_state_idle;
  bool system_state_chargecompleted;
  bool system_state_maintenancecharge;
  bool IO_state_main_positive_relay;
  bool IO_state_main_negative_relay;
  bool IO_state_charge_enable;
  bool IO_state_precharge_relay;
  bool IO_state_discharge_enable;
  bool IO_state_IO_6;
  bool IO_state_IO_7;
  bool IO_state_IO_8;
  bool error_Cell_overvoltage;
  bool error_Cell_undervoltage;
  bool error_Cell_end_of_life_voltage;
  bool error_Cell_voltage_misread;
  bool error_Cell_over_temperature;
  bool error_Cell_under_temperature;
  bool error_Cell_unmanaged;
  bool error_LMU_over_temperature;
  bool error_LMU_under_temperature;
  bool error_Temp_sensor_open_circuit;
  bool error_Temp_sensor_short_circuit;
  bool error_SUB_communication;
  bool error_LMU_communication;
  bool error_Over_current_IN;
  bool error_Over_current_OUT;
  bool error_Short_circuit;
  bool error_Leak_detected;
  bool error_Leak_detection_failed;
  bool error_Voltage_difference;
  bool error_BMCU_supply_over_voltage;
  bool error_BMCU_supply_under_voltage;
  bool error_Main_positive_contactor;
  bool error_Main_negative_contactor;
  bool error_Precharge_contactor;
  bool error_Midpack_contactor;
  bool error_Precharge_timeout;
  bool error_Emergency_connector_override;
  bool warning_High_cell_voltage;
  bool warning_Low_cell_voltage;
  bool warning_High_cell_temperature;
  bool warning_Low_cell_temperature;
  bool warning_High_LMU_temperature;
  bool warning_Low_LMU_temperature;
  bool warning_SUB_communication_interfered;
  bool warning_LMU_communication_interfered;
  bool warning_High_current_IN;
  bool warning_High_current_OUT;
  bool warning_Pack_resistance_difference;
  bool warning_High_pack_resistance;
  bool warning_Cell_resistance_difference;
  bool warning_High_cell_resistance;
  bool warning_High_BMCU_supply_voltage;
  bool warning_Low_BMCU_supply_voltage;
  bool warning_Low_SOC;
  bool warning_Balancing_required_OCV_model;
  bool warning_Charger_not_responding;
};

struct DATALAYER_INFO_CHADEMO {
  uint8_t CHADEMO_Status;
  uint8_t ControlProtocolNumberEV;
  bool UserRequestRestart;
  bool UserRequestStop;
  bool FaultBatteryVoltageDeviation;
  bool FaultHighBatteryTemperature;
  bool FaultBatteryCurrentDeviation;
  bool FaultBatteryUnderVoltage;
  bool FaultBatteryOverVoltage;
};

struct DATALAYER_INFO_CMFAEV {
  uint64_t cumulative_energy_when_discharging;
  uint64_t cumulative_energy_when_charging;
  uint64_t cumulative_energy_in_regen;

  uint32_t average_voltage_of_cells;

  uint16_t soc_z;
  uint16_t soc_u;
  uint16_t soh_average;
  uint16_t max_regen_power;
  uint16_t max_discharge_power;
  uint16_t maximum_charge_power;
  uint16_t SOH_available_power;
  uint16_t SOH_generated_power;
  uint16_t lead_acid_voltage;

  int16_t average_temperature;
  int16_t minimum_temperature;
  int16_t maximum_temperature;

  uint8_t highest_cell_voltage_number;
  uint8_t lowest_cell_voltage_number;
};

struct DATALAYER_INFO_CMPSMART {
  uint8_t battery_negative_contactor_state;
  uint8_t battery_precharge_contactor_state;
  uint8_t battery_positive_contactor_state;
  uint8_t battery_state;
  uint8_t eplug_status;
  uint8_t HVIL_status;
  uint8_t ev_warning;
  uint8_t insulation_fault;
  uint8_t insulation_circuit_status;
  uint8_t hardware_fault_status;
  uint8_t l3_fault;
  uint8_t plausibility_error;
  uint8_t battery_charging_status;
  uint8_t battery_fault;
  uint8_t hvbat_wakeup_state;
  uint8_t active_DTC_code;
  uint8_t alert_frame3;
  uint8_t alert_frame4;
  bool rcd_line_active;
  bool power_auth;
  bool battery_balancing_active;
  bool UserRequestDTCreset; /** User requesting DTC reset via WebUI*/
};

struct DATALAYER_INFO_ECMP {

  uint32_t pid_insulation_res_neg;
  uint32_t pid_insulation_res_pos;
  uint32_t pid_max_current_10s;
  uint32_t pid_max_discharge_10s;
  uint32_t pid_max_discharge_30s;
  uint32_t pid_max_charge_10s;
  uint32_t pid_max_charge_30s;
  uint32_t pid_energy_capacity;
  uint32_t pid_insulation_res;
  uint32_t pid_crash_counter;
  uint32_t pid_history_data;
  uint32_t pid_last_can_failure_detail;
  uint32_t pid_hw_version_num;
  uint32_t pid_sw_version_num;
  uint32_t pid_current_time;
  uint32_t pid_time_sent_by_car;
  uint32_t pid_vehicle_speed;
  uint32_t pid_time_spent_over_55c;
  uint32_t pid_contactor_closing_counter;
  uint32_t pid_date_of_manufacture;

  int32_t pid_current;

  uint16_t pid_most_critical_fault;
  uint16_t HV_BATT_FC_INSU_MINUS_RES;     //mysteryvan parameters
  uint16_t HV_BATT_FC_INSU_PLUS_RES;      //mysteryvan parameters
  uint16_t HV_BATT_FC_VHL_INSU_PLUS_RES;  //mysteryvan parameters
  uint16_t HV_BATT_ONLY_INSU_MINUS_RES;   //mysteryvan parameters
  uint16_t InsulationResistance;
  uint16_t pid_avg_cell_voltage;
  uint16_t pid_lowsoc_counter;
  uint16_t pid_sum_of_cells;
  uint16_t pid_cell_min_capacity;
  uint16_t pid_pack_voltage;
  uint16_t pid_high_cell_voltage;
  uint16_t pid_low_cell_voltage;
  uint16_t pid_SOH_cell_1;
  uint16_t pid_12v;
  uint16_t pid_hvil_in_voltage;
  uint16_t pid_hvil_out_voltage;

  uint8_t pid_bms_state;
  uint8_t pid_hvil_state;
  uint8_t pid_mainfuse_state;
  uint8_t pid_precharge_short_circuit;
  uint8_t pid_eservice_plug_state;
  uint8_t pid_battery_state;
  uint8_t pid_aux_fuse_state;
  uint8_t pid_12v_abnormal;
  uint8_t InsulationDiag;
  uint8_t MainConnectorState;
  uint8_t CONTACTOR_OPENING_REASON;  //mysteryvan parameters
  uint8_t TBMU_FAULT_TYPE;           //mysteryvan parameters
  uint8_t CONTACTORS_STATE;          //mysteryvan parameters
  uint8_t pid_factory_mode_control;
  uint8_t pid_welding_detection;
  uint8_t pid_reason_open;
  uint8_t pid_contactor_status;
  uint8_t pid_negative_contactor_control;
  uint8_t pid_negative_contactor_status;
  uint8_t pid_positive_contactor_control;
  uint8_t pid_positive_contactor_status;
  uint8_t pid_contactor_negative;
  uint8_t pid_contactor_positive;
  uint8_t pid_precharge_relay_control;
  uint8_t pid_precharge_relay_status;
  uint8_t pid_recharge_status;
  uint8_t pid_coldest_module;
  uint8_t pid_hottest_module;
  uint8_t pid_battery_energy;
  uint8_t pid_wire_crash;
  uint8_t pid_CAN_crash;
  uint8_t pid_highest_cell_voltage_num;
  uint8_t pid_lowest_cell_voltage_num;
  uint8_t pid_cell_voltage_measurement_status;

  int8_t pid_delta_temperature;
  int8_t pid_lowest_temperature;
  int8_t pid_average_temperature;
  int8_t pid_highest_temperature;

  bool MysteryVan;      //mysteryvan parameters
  bool CrashMemorized;  //mysteryvan parameters
  bool InterlockOpen;
  bool ALERT_CELL_POOR_CONSIST;  //mysteryvan parameters
  bool ALERT_OVERCHARGE;         //mysteryvan parameters
  bool ALERT_BATT;               //mysteryvan parameters
  bool ALERT_LOW_SOC;            //mysteryvan parameters
  bool ALERT_HIGH_SOC;           //mysteryvan parameters
  bool ALERT_SOC_JUMP;           //mysteryvan parameters
  bool ALERT_TEMP_DIFF;          //mysteryvan parameters
  bool ALERT_HIGH_TEMP;          //mysteryvan parameters
  bool ALERT_OVERVOLTAGE;        //mysteryvan parameters
  bool ALERT_CELL_OVERVOLTAGE;   //mysteryvan parameters
  bool ALERT_CELL_UNDERVOLTAGE;  //mysteryvan parameters

  uint8_t pid_battery_serial[13];
};

struct DATALAYER_INFO_FORD_MACH_E {
  int16_t pid_hvb_temp;
  uint32_t pid_hvb_soc;
  uint32_t pid_hvb_contactor_status;
  uint16_t pid_hvb_contactor_positive_leak_voltage;
  uint16_t pid_hvb_contactor_negative_leak_voltage;
  uint16_t pid_hvb_contactor_positive_voltage;
  uint16_t pid_hvb_contactor_negative_voltage;
  uint16_t pid_hvb_contactor_positive_bus_leak_resistance;
  uint16_t pid_hvb_contactor_negative_bus_leak_resistance;
  uint16_t pid_hvb_contactor_overall_leak_resistance;
  uint16_t pid_hvb_contactor_open_leak_resistance;
  uint16_t pid_hvb_voltage;
  uint16_t pid_hvb_max_charge_current;
  uint16_t discharge_power_cand2;
  uint16_t discharge_power_cand1;
  uint16_t charge_power_cand2;
  uint16_t charge_power_cand1;
  uint16_t pid_hvb_calendar_age_months;
  uint16_t pid_battery_capacity_ah;
  uint8_t pid_maintenance_rebalance_status;
  uint8_t pid_hvb_soh;
};

struct DATALAYER_INFO_GEELY_GEOMETRY_C {
  /** int16_t */
  /** Module temperatures 1-6 */
  int16_t ModuleTemperatures[6];

  /** uint8_t */
  /** Battery software/hardware/serial versions, stores raw HEX values for ASCII chars */
  uint8_t BatterySoftwareVersion[16];
  uint8_t BatteryHardwareVersion[16];
  uint8_t BatterySerialNumber[28];

  /** uint16_t */
  /** Various values polled via OBD2 PIDs */
  uint16_t soc;
  uint16_t CC2voltage;
  uint16_t cellMaxVoltageNumber;
  uint16_t cellMinVoltageNumber;
  uint16_t cellTotalAmount;
  uint16_t specificialVoltage;
  uint16_t unknown1;
  uint16_t rawSOCmax;
  uint16_t rawSOCmin;
  uint16_t unknown4;
  uint16_t capModMax;
  uint16_t capModMin;
  uint16_t unknown7;
  uint16_t unknown8;
};

struct DATALAYER_INFO_KIAHYUNDAI64 {
  uint32_t cumulative_charge_current_ah;
  uint32_t cumulative_discharge_current_ah;
  uint32_t cumulative_energy_charged_kWh;
  uint32_t cumulative_energy_discharged_kWh;
  uint32_t powered_on_total_time;

  uint16_t inverterVoltage;
  uint16_t isolation_resistance_kOhm;
  uint16_t number_of_standard_charging_sessions;
  uint16_t number_of_fastcharging_sessions;
  uint16_t accumulated_normal_charging_energy_kWh;
  uint16_t accumulated_fastcharging_energy_kWh;
  uint16_t battery_12V;

  int8_t temperature_water_inlet;
  int8_t powerRelayTemperature;

  uint8_t total_cell_count;
  uint8_t waterleakageSensor;
  uint8_t batteryManagementMode;
  uint8_t BMS_ign;
  uint8_t batteryRelay;

  uint8_t ecu_serial_number[16];
  uint8_t ecu_version_number[16];
};

struct DATALAYER_INFO_RIVIAN {
  uint16_t pre_contactor_voltage;
  uint16_t main_contactor_voltage;
  uint16_t voltage_reference;
  uint16_t DCFC_contactor_voltage;
  uint16_t battery_SOC_max;
  uint16_t battery_SOC_min;
  uint8_t BMS_state;
  uint8_t HVIL;
  uint8_t error_flags_from_BMS;
  uint8_t contactor_state;
  uint8_t HMI_part1;
  uint8_t HMI_part2;
  uint8_t isolation_fault_status;
  uint8_t system_safe_state;
  bool error_relay_open;
  bool IsolationMeasurementOngoing;
  bool puncture_fault;
  bool liquid_fault;
  bool contactor_DCFC_welded;
  bool NACS_charger_detected;
  bool slewrate_potential_violation;
  bool minimum_power_potential_violation;
  bool operation_limit_violation_warning;
};

struct DATALAYER_INFO_TESLA {
  // Alert-matrix (DTC) fault flags for the Faults web page; mapped from tesla-m3-pack-findings (fw 2019.20.4.2)
  bool BMS_alertMatrixActive[100];
  bool PCS_alertMatrixActive[94];
  bool CP_alertMatrixActive[96];

  uint64_t BMS_info_bootGitHash;
  uint64_t PCS_info_bootGitHash;
  uint64_t HVP_info_bootGitHash;

  uint32_t HVP_info_bootCrc;
  uint32_t HVP_info_appCrc;
  uint32_t PCS_info_appCrc;
  uint32_t PCS_info_cpu2AppCrc;
  uint32_t PCS_info_bootCrc;
  uint32_t PCS_dcdc12vSupportLifetimekWh;
  uint32_t BMS_info_appCrc;
  uint32_t BMS_info_bootCrc;
  uint32_t battery_packMass;
  uint32_t battery_platformMaxBusVoltage;
  uint32_t BMS_min_voltage;
  uint32_t BMS_max_voltage;
  uint32_t battery_max_charge_current;
  uint32_t battery_max_discharge_current;
  uint32_t battery_soc_min;
  uint32_t battery_soc_max;
  uint32_t battery_soc_ave;
  uint32_t battery_soc_ui;

  uint16_t BMS_info_buildConfigId;
  uint16_t BMS_info_hardwareId;
  uint16_t BMS_info_componentId;
  uint16_t BMS_info_usageId;
  uint16_t BMS_info_subUsageId;
  uint16_t battery_dcdcLvBusVolt;
  uint16_t battery_dcdcHvBusVolt;
  uint16_t battery_dcdcLvOutputCurrent;
  uint16_t battery_nominal_full_pack_energy;
  uint16_t battery_nominal_full_pack_energy_m0;
  uint16_t battery_nominal_energy_remaining;
  uint16_t battery_nominal_energy_remaining_m0;
  uint16_t battery_ideal_energy_remaining;
  uint16_t battery_ideal_energy_remaining_m0;
  uint16_t battery_energy_to_charge_complete;
  uint16_t battery_energy_to_charge_complete_m1;
  uint16_t battery_energy_buffer;
  uint16_t battery_energy_buffer_m1;
  uint16_t battery_expected_energy_remaining;
  uint16_t battery_expected_energy_remaining_m1;
  uint16_t battery_BrickVoltageMax;
  uint16_t battery_BrickVoltageMin;
  uint16_t HVP_hvp1v5Ref;
  uint16_t HVP_shuntCurrentDebug;
  int16_t PCS_dcdcTemp;
  int16_t PCS_ambientTemp;
  int16_t PCS_chgPhATemp;
  int16_t PCS_chgPhBTemp;
  int16_t PCS_chgPhCTemp;
  uint16_t PCS_dcdcMaxLvOutputCurrent;
  uint16_t PCS_dcdcCurrentLimit;
  uint16_t PCS_dcdcLvOutputCurrentTempLimit;
  uint16_t PCS_dcdcUnifiedCommand;
  uint16_t PCS_dcdcCLAControllerOutput;
  uint16_t PCS_dcdcTankVoltage;
  uint16_t PCS_dcdcTankVoltageTarget;
  uint16_t PCS_dcdcClaCurrentFreq;
  uint16_t PCS_dcdcTCommMeasured;
  uint16_t PCS_dcdcShortTimeUs;
  uint16_t PCS_dcdcHalfPeriodUs;
  uint16_t PCS_dcdcIntervalMaxFrequency;
  uint16_t PCS_dcdcIntervalMaxHvBusVolt;
  uint16_t PCS_dcdcIntervalMaxLvBusVolt;
  uint16_t PCS_dcdcIntervalMaxLvOutputCurr;
  uint16_t PCS_dcdcIntervalMinFrequency;
  uint16_t PCS_dcdcIntervalMinHvBusVolt;
  uint16_t PCS_dcdcIntervalMinLvBusVolt;
  uint16_t PCS_dcdcIntervalMinLvOutputCurr;
  uint16_t battery_packConfigMultiplexer;
  uint16_t battery_moduleType;
  uint16_t battery_reservedConfig;
  uint16_t BMS_isolationResistance;
  uint16_t BMS_chgPowerAvailable;
  uint16_t BMS_maxRegenPower;
  uint16_t BMS_maxDischargePower;
  uint16_t BMS_maxStationaryHeatPower;
  uint16_t BMS_hvacPowerBudget;
  uint16_t BMS_powerDissipation;
  uint16_t BMS_inletActiveCoolTargetT;
  uint16_t BMS_inletPassiveTargetT;
  uint16_t BMS_inletActiveHeatTargetT;
  uint16_t BMS_packTMin;
  uint16_t BMS_packTMax;
  uint16_t PCS_info_buildConfigId;
  uint16_t PCS_info_hardwareId;
  uint16_t PCS_info_componentId;
  uint16_t PCS_dcdcMaxOutputCurrentAllowed;
  uint16_t PCS_info_usageId;
  uint16_t PCS_info_subUsageId;
  uint16_t HVP_dcLinkVoltage;
  uint16_t HVP_packVoltage;
  uint16_t HVP_fcLinkVoltage;
  uint16_t HVP_packContVoltage;
  uint16_t HVP_packNegativeV;
  uint16_t HVP_packPositiveV;
  uint16_t HVP_pyroAnalog;
  uint16_t HVP_dcLinkNegativeV;
  uint16_t HVP_dcLinkPositiveV;
  uint16_t HVP_fcLinkNegativeV;
  uint16_t HVP_fcContCoilCurrent;
  uint16_t HVP_fcContVoltage;
  uint16_t HVP_hvilInVoltage;
  uint16_t HVP_hvilOutVoltage;
  uint16_t HVP_fcLinkPositiveV;
  uint16_t HVP_packContCoilCurrent;
  uint16_t HVP_battery12V;
  uint16_t HVP_shuntRefVoltageDbg;
  uint16_t HVP_shuntAuxCurrentDbg;
  uint16_t HVP_shuntBarTempDbg;
  uint16_t HVP_shuntAsicTempDbg;
  uint16_t HVP_info_buildConfigId;
  uint16_t HVP_info_hardwareId;
  uint16_t HVP_info_componentId;
  uint16_t HVP_info_usageId;
  uint16_t HVP_info_subUsageId;

  uint8_t hvil_status;
  uint8_t packContNegativeState;
  uint8_t packContPositiveState;
  uint8_t packContactorSetState;
  uint8_t battery_packCtrsRequestStatus;
  uint8_t BMS_info_pcbaId;
  uint8_t BMS_info_assemblyId;
  uint8_t BMS_info_platformType;
  uint8_t BMS_info_bootUdsProtoVersion;
  uint8_t battery_beginning_of_life;
  uint8_t battery_battTempPct;
  uint8_t battery_BrickVoltageMaxNum;
  uint8_t battery_BrickVoltageMinNum;
  uint8_t battery_BrickTempMaxNum;
  uint8_t battery_BrickTempMinNum;
  uint8_t battery_BrickModelTMax;
  uint8_t battery_BrickModelTMin;
  uint8_t BMS_flowRequest;
  uint8_t BMS_uiChargeStatus;
  uint8_t BMS_contactorState;
  uint8_t BMS_state;
  uint8_t BMS_hvState;
  uint8_t BMS_notEnoughPowerForHeatPump;
  uint8_t BMS_powerLimitState;
  uint8_t BMS_inverterTQF;
  uint8_t PCS_dcdcPrechargeStatus;
  uint8_t PCS_dcdc12VSupportStatus;
  uint8_t PCS_dcdcHvBusDischargeStatus;
  uint8_t PCS_dcdcMainState;
  uint8_t PCS_dcdcSubState;
  uint8_t PCS_dcdcPrechargeRtyCnt;
  uint8_t PCS_dcdc12VSupportRtyCnt;
  uint8_t PCS_dcdcDischargeRtyCnt;
  uint8_t PCS_dcdcPwmEnableLine;
  uint8_t PCS_dcdcSupportingFixedLvTarget;
  uint8_t PCS_dcdcPrechargeRestartCnt;
  uint8_t PCS_dcdcInitialPrechargeSubState;
  uint8_t PCS_info_pcbaId;
  uint8_t PCS_info_assemblyId;
  uint8_t PCS_info_platformType;
  uint8_t PCS_info_bootUdsProtoVersion;
  uint8_t HVP_info_platformType;
  uint8_t HVP_info_pcbaId;
  uint8_t HVP_info_assemblyId;
  uint8_t HVP_info_bootUdsProtoVersion;
  uint8_t HVP_shuntHwMia;
  uint8_t HVP_shuntAuxCurrentStatus;
  uint8_t HVP_shuntBarTempStatus;
  uint8_t HVP_shuntAsicTempStatus;

  bool packCtrsClosingBlocked;
  bool pyroTestInProgress;
  bool battery_packCtrsOpenNowRequested;
  bool battery_packCtrsOpenRequested;
  bool battery_packCtrsResetRequestRequired;
  bool battery_dcLinkAllowedToEnergize;
  bool BMS352_mux;  // variable to store when 0x352 mux is present
  bool battery_full_charge_complete;
  bool battery_fully_charged;
  bool BMS_hvilFault;
  bool BMS_diLimpRequest;
  bool BMS_pcsPwmEnabled;
  bool BMS_pcsNoFlowRequest;
  bool BMS_noFlowRequest;
  bool PCS_dcdcFaulted;
  bool PCS_dcdcOutputIsLimited;
  bool HVP_gpioPassivePyroDepl;
  bool HVP_gpioPyroIsoEn;
  bool HVP_gpioCpFaultIn;
  bool HVP_gpioPackContPowerEn;
  bool HVP_gpioHvCablesOk;
  bool HVP_gpioHvpSelfEnable;
  bool HVP_gpioLed;
  bool HVP_gpioCrashSignal;
  bool HVP_gpioShuntDataReady;
  bool HVP_gpioFcContPosAux;
  bool HVP_gpioFcContNegAux;
  bool HVP_gpioBmsEout;
  bool HVP_gpioCpFaultOut;
  bool HVP_gpioPyroPor;
  bool HVP_gpioShuntEn;
  bool HVP_gpioHvpVerEn;
  bool HVP_gpioPackCoontPosFlywheel;
  bool HVP_gpioCpLatchEnable;
  bool HVP_gpioPcsEnable;
  bool HVP_gpioPcsDcdcPwmEnable;
  bool HVP_gpioPcsChargePwmEnable;
  bool HVP_gpioFcContPowerEnable;
  bool HVP_gpioHvilEnable;
  bool HVP_gpioSecDrdy;
  bool HVP_packCurrentMia;
  bool HVP_auxCurrentMia;
  bool HVP_currentSenseMia;
  bool HVP_shuntRefVoltageMismatch;
  bool HVP_shuntThermistorMia;

  uint8_t BMS_partNumber[12];        //stores raw HEX values for ASCII chars
  uint8_t battery_serialNumber[15];  //stores raw HEX values for ASCII chars
  uint8_t battery_partNumber[12];    //stores raw HEX values for ASCII chars
  uint8_t PCS_partNumber[13];        //stores raw HEX values for ASCII chars
  uint8_t HVP_partNumber[13];        //stores raw HEX values for ASCII chars
  char* battery_manufactureDate;
};

struct DATALAYER_INFO_NISSAN_LEAF {
  /** Cryptographic challenge to be solved */
  uint32_t CryptoChallenge;
  /** Solution for crypto challenge, MSBs */
  uint32_t SolvedChallengeMSB;
  /** Solution for crypto challenge, LSBs */
  uint32_t SolvedChallengeLSB;

  /** 77Wh per gid. LEAF specific unit */
  uint16_t GIDS;
  /** Max regen power in kW */
  uint16_t ChargePowerLimit;
  /** Internal resistance in percentage */
  uint16_t battery_HX;
  /** Insulation resistance, most likely kOhm */
  uint16_t Insulation;

  /** Max charge power in kW */
  int16_t MaxPowerForCharger;
  /** Temperature sensoros 1-4 */
  int16_t temperature1;
  int16_t temperature2;
  int16_t temperature3;  // This sensor not available on 2013+ packs
  int16_t temperature4;

  /** Enum, ZE0, AZE0 = 1, ZE1 = 2 */
  uint8_t LEAF_gen;
  /** battery_FAIL status */
  uint8_t RelayCutRequest;
  /** battery_STATUS status */
  uint8_t FailsafeStatus;

  /** Interlock status */
  bool Interlock;
  /** True if fully charged */
  bool Full;
  /** True if battery empty */
  bool Empty;
  /** Battery pack allows closing of contacors */
  bool MainRelayOn;
  /** True if heater exists */
  bool HeatExist;
  /** Heater stopped */
  bool HeatingStop;
  /** Heater starting */
  bool HeatingStart;
  /** Heat request sent*/
  bool HeaterSendRequest;
  /** True if the crypto challenge response from BMS is signalling a failed attempt*/
  bool challengeFailed;

  /** Battery info, stores raw HEX values for ASCII chars */
  uint8_t BatterySerialNumber[15];
  uint8_t BatteryPartNumber[7];
  uint8_t BMSIDcode[8];
};

struct DATALAYER_INFO_MEB {
  /** Isolation resistance in kOhm */
  uint32_t isolation_resistance;
  int32_t BMS_voltage_intermediate_dV;
  int32_t BMS_voltage_dV;

  uint16_t battery_temperature_dC;

  /** All realtime_ warnings have same enumeration, 0 = no fault, 1 = error level 1, 2 error level 2, 3 error level 3 */
  uint8_t rt_overcurrent;
  uint8_t rt_CAN_fault;
  uint8_t rt_overcharge;
  uint8_t rt_SOC_high;
  uint8_t rt_SOC_low;
  uint8_t rt_SOC_jumping;
  uint8_t rt_temp_difference;
  uint8_t rt_cell_overtemp;
  uint8_t rt_cell_undertemp;
  uint8_t rt_battery_overvolt;
  uint8_t rt_battery_undervol;
  uint8_t rt_cell_overvolt;
  uint8_t rt_cell_undervol;
  uint8_t rt_cell_imbalance;
  uint8_t rt_battery_unathorized;
  /** HVIL status, 0 = Init, 1 = Closed, 2 = Open!, 3 = Fault */
  uint8_t HVIL;
  /** 0 = HV inactive, 1 = HV active, 2 = Balancing, 3 = Extern charging, 4 = AC charging, 5 = Battery error, 6 = DC charging, 7 = init */
  uint8_t BMS_mode;
  /** 1 = Battery display, 4 = Battery display OK, 4 = Display battery charging, 6 = Display battery check, 7 = Fault */
  uint8_t battery_diagnostic;
  /** 0 = init, 1 = no open HV line detected, 2 = open HV line , 3 = fault (PTC Heater HV connector)*/
  uint8_t status_HV_PTC_line;
  /** 0 = OK, 1 = Not OK, 0x06 = init, 0x07 = fault */
  uint8_t warning_support;
  /** 0=Init, 1=BMS intermediate circuit voltage-free (U_Zwkr < 20V), 2=BMS intermediate circuit not voltage-free (U_Zwkr >/= 25V, hysteresis), 3=Error */
  uint8_t BMS_status_voltage_free;
  /** 0 Component_IO, 1 Restricted_CompFkt_Isoerror_I, 2 Restricted_CompFkt_Isoerror_II, 3 Restricted_CompFkt_Interlock, 4 Restricted_CompFkt_SD, 5 Restricted_CompFkt_Performance red, 6 = No component function, 7 = Init */
  uint8_t BMS_error_status;
  /** 0 init, 1 closed, 2 open, 3 fault */
  uint8_t BMS_Kl30c_Status;
  uint8_t balancing_active;
  uint8_t BMS_welded_contactors_status;

  bool balancing_request;
  bool charging_active;
  bool BMS_OBD_MIL; /** true if BMS requests error/warning light */
  bool BMS_error_lamp_req;
  bool BMS_warning_lamp_req;
  bool BMS_fault_performance;  //Error: Battery performance is limited (e.g. due to sensor or fan failure)
  bool
      BMS_fault_emergency_shutdown_crash;  //Error: Safety-critical error (crash detection) Battery contactors are already opened / will be opened immediately Signal is read directly by the EMS and initiates an AKS of the PWR and an active discharge of the DC link
  bool
      BMS_error_shutdown_request;  // Fault: Fault condition, requires battery contactors to be opened internal battery error; Advance notification of an impending opening of the battery contactors by the BMS
  bool
      BMS_error_shutdown;  // Fault: Fault condition, requires battery contactors to be opened Internal battery error, battery contactors opened without notice by the BMS
  bool SDSW;               /** Service disconnect switch status */
  bool pilotline;          /** Pilotline status */
  bool transportmode;      /** Transportation mode status */
  bool componentprotection; /** Componentprotection mode status */
  bool shutdown_active;     /** Shutdown status */
  bool battery_heating;     /** Battery heating status */

  float temp_points[18];
  int16_t celltemperature_dC[56];

  // DTC readout (UDS 0x19 0x02) and clear (OBD service 0x04). Codes stored as raw 3 bytes
  // packed in a uint32, rendered to string in HTML.
  bool dtc_read_in_progress = false;   // Flag to prevent concurrent reads
  bool UserRequestDTCreset = false;    // User requesting DTC erase via WebUI
  bool UserRequestDTCreadout = false;  // User requesting DTC readout via WebUI
  bool UserRequestBMSReset = false;    // User requesting BMS reset via WebUI
};

struct DATALAYER_INFO_VOLVO_POLESTAR {
  uint16_t BECMsupplyVoltage;
  uint16_t BECMUDynMaxLim;
  uint16_t BECMUDynMinLim;
  uint16_t HvBattPwrLimDcha1;
  uint16_t HvBattPwrLimDchaSoft;
  uint16_t HvBattPwrLimDchaSlowAgi;
  uint16_t HvBattPwrLimChrgSlowAgi;

  uint8_t HVSysRlySts;
  uint8_t HVSysDCRlySts1;
  uint8_t HVSysDCRlySts2;
  uint8_t HVSysIsoRMonrSts;
  uint8_t DTCcount;
  uint8_t HVILstatusBits;
  /** User requesting DTC reset via WebUI*/
  bool UserRequestDTCreset;
  /** User requesting DTC readout via WebUI*/
  bool UserRequestDTCreadout;
  /** User requesting BECM reset via WebUI*/
  bool UserRequestBECMecuReset;
};

struct DATALAYER_INFO_VOLVO_HYBRID {
  uint16_t soc_bms;
  uint16_t soc_calc;
  uint16_t soc_rescaled;
  uint16_t soh_bms;
  uint16_t BECMsupplyVoltage;

  uint16_t BECMBatteryVoltage;
  uint16_t BECMBatteryCurrent;
  uint16_t BECMUDynMaxLim;
  uint16_t BECMUDynMinLim;

  uint16_t HvBattPwrLimDcha1;
  uint16_t HvBattPwrLimDchaSoft;
  //uint16_t HvBattPwrLimDchaSlowAgi;
  //uint16_t HvBattPwrLimChrgSlowAgi;

  uint8_t HVSysRlySts;
  uint8_t HVSysDCRlySts1;
  uint8_t HVSysDCRlySts2;
  uint8_t HVSysIsoRMonrSts;
  /** User requesting DTC reset via WebUI*/
  bool UserRequestDTCreset;
  /** User requesting DTC readout via WebUI*/
  bool UserRequestDTCreadout;
  /** User requesting BECM reset via WebUI*/
  bool UserRequestBECMecuReset;
};

struct DATALAYER_INFO_GEELY_SEA {
  uint16_t soc_bms;
  uint16_t soh_bms;
  uint16_t BECMsupplyVoltage;
  uint16_t BECMBatteryVoltage;
  uint16_t BatteryCurrent;
  uint16_t CellTempHighest;
  uint16_t CellTempAverage;
  uint16_t CellTempLowest;
  uint8_t Interlock;
  uint16_t CellVoltHighest;
  uint16_t CellVoltLowest;
  uint8_t DTCcount;
  uint8_t CrashStatus;
  /** User requesting DTC reset via WebUI*/
  bool UserRequestDTCreset;
  /** User requesting DTC readout via WebUI*/
  bool UserRequestDTCreadout;
  /** User requesting BECM reset via WebUI*/
  bool UserRequestBECMecuReset;
  /** User requesting reset of crash status via WebUI*/
  bool UserRequestCrashReset;
};

struct DATALAYER_INFO_ZOE {
  uint16_t mileage_km;
  uint16_t alltime_kWh;

  uint8_t CUV;
  uint8_t HVBIR;
  uint8_t HVBUV;
  uint8_t EOCR;
  uint8_t HVBOC;
  uint8_t HVBOT;
  uint8_t HVBOV;
  uint8_t COV;
};

struct DATALAYER_INFO_ZOE_PH2 {
  uint32_t battery_slave_failures;
  /** uint16_t */
  uint16_t battery_soc;
  uint16_t battery_usable_soc;
  uint16_t battery_soh;
  uint16_t battery_pack_voltage;
  uint16_t battery_max_cell_voltage;
  uint16_t battery_min_cell_voltage;
  uint16_t battery_12v;
  uint16_t battery_avg_temp;
  uint16_t battery_min_temp;
  uint16_t battery_max_temp;
  uint16_t battery_max_power;
  uint16_t battery_interlock;
  uint16_t battery_kwh;
  uint16_t battery_current;
  uint16_t battery_current_offset;
  uint16_t battery_max_generated;
  uint16_t battery_max_available;
  uint16_t battery_current_voltage;
  uint16_t battery_charging_status;
  uint16_t battery_remaining_charge;
  uint16_t battery_balance_capacity_total;
  uint16_t battery_balance_time_total;
  uint16_t battery_balance_capacity_sleep;
  uint16_t battery_balance_time_sleep;
  uint16_t battery_balance_capacity_wake;
  uint16_t battery_balance_time_wake;
  uint16_t battery_bms_state;
  uint16_t battery_energy_complete;
  uint16_t battery_energy_partial;
  uint16_t battery_mileage;
  uint16_t battery_fan_speed;
  uint16_t battery_fan_period;
  uint16_t battery_fan_control;
  uint16_t battery_fan_duty;
  uint16_t battery_temporisation;
  uint16_t battery_time;
  uint16_t battery_pack_time;
  uint16_t battery_soc_min;
  uint16_t battery_soc_max;
  /** User requesting NVROL reset via WebUI*/
  bool UserRequestNVROLReset;
};

class DataLayerExtended {
 public:
  union {
    // All zero-initialized entries should go inside this union.
    // Double-battery repeats should go inside their own structs.

    struct {
      DATALAYER_INFO_BOLTAMPERA boltampera;
      DATALAYER_INFO_BOLTAMPERA boltampera_2;
    };
    DATALAYER_INFO_BMWPHEV bmwphev;
    DATALAYER_INFO_BMWIX bmwix;
    DATALAYER_INFO_CELLPOWER cellpower;
    DATALAYER_INFO_CHADEMO chademo;
    DATALAYER_INFO_CMFAEV CMFAEV;
    DATALAYER_INFO_CMPSMART stellantisCMPsmart;
    DATALAYER_INFO_ECMP stellantisECMP;
    DATALAYER_INFO_FORD_MACH_E fordMachE;
    DATALAYER_INFO_GEELY_GEOMETRY_C geometryC;
    struct {
      DATALAYER_INFO_KIAHYUNDAI64 KiaHyundai64;
      DATALAYER_INFO_KIAHYUNDAI64 KiaHyundai64_2;
    };
    DATALAYER_INFO_TESLA tesla;
    DATALAYER_INFO_NISSAN_LEAF nissanleaf;
    DATALAYER_INFO_MEB meb;
    DATALAYER_INFO_VOLVO_HYBRID VolvoHybrid;
    DATALAYER_INFO_ZOE zoe;
  };

  // Entries with non-zero default values should go here.
  DATALAYER_INFO_BYDATTO3 bydAtto3;
  DATALAYER_INFO_BYDATTO3 bydAtto3_2;
  DATALAYER_INFO_RIVIAN rivian;
  DATALAYER_INFO_VOLVO_POLESTAR VolvoPolestar;
  DATALAYER_INFO_GEELY_SEA GeelySEA;
  DATALAYER_INFO_ZOE_PH2 zoePH2;

  DataLayerExtended() {
    memset(this, 0, sizeof(DataLayerExtended));

    // Non-zero defaults should be initialized here.

    auto initBydAtto3 = [](auto& data) {
      data.calibrationTargetSOC = 100;
      data.calibrationTargetAH = 150;
      data.discharge_status = 14;
      data.auto_calibrate_soc_enabled = true;
      data.auto_calibrate_soc_drift_percent = 5;
    };
    initBydAtto3(bydAtto3);
    initBydAtto3(bydAtto3_2);
  }
};

extern DataLayerExtended datalayer_extended;

#endif
