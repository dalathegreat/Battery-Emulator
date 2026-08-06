export const DATALAYER_INFO_BYDATTO3_FIELDS: ([string, string] | [string, string, number])[] = [
  ['SOC_estimated', 'u16'],
  ['SOC_highprec', 'u16'],
  ['SOC_polled', 'u16'],
  ['pack_voltage_dV', 'u16'],
  ['insulation_ohm_per_volt', 'u16'],
  ['insulation_valid', 'b'],
  ['', ' '],
  ['chargePower', 'u16'],
  ['charge_times', 'u16'],
  ['dischargePower', 'u16'],
  ['total_charged_ah', 'u16'],
  ['total_discharged_ah', 'u16'],
  ['total_charged_kwh', 'u16'],
  ['total_discharged_kwh', 'u16'],
  ['times_full_power', 'u16'],
  ['BMS_capacity_original_calibration', 'u16'],
  ['BMC_SOC_original_calibration', 'u16'],
  ['BMS_capacity_current_calibration', 'u16'],
  ['BMC_SOC_current_calibration', 'u16'],
  ['seed', 'u16'],
  ['solvedKey', 'u16'],
  ['calibrationTargetSOC', 'u16'],
  ['calibrationTargetAH', 'u16'],
  ['battery_temperatures', 'i16', 13],
  ['discharge_status', 'u8'],
  ['BMS_min_cell_voltage_number', 'u8'],
  ['BMS_min_temp_module_number', 'u8'],
  ['BMS_max_cell_voltage_number', 'u8'],
  ['BMS_max_temp_module_number', 'u8'],
  ['servicemode', 'u8'],
  ['UserRequestCrashReset', 'b'],
  ['UserRequestCalibrateSOC', 'b'],
  ['contactor_control_state', 'u8'],
  ['contactor_feedback', 'u8'],
  ['contactor_main_closed', 'b'],
  ['contactor_precharging', 'b'],
  ['contactor_hv_active', 'b'],
  ['contactor_drive_flag', 'b'],
  ['contactor_charge_flag', 'b'],
  ['auto_calibrate_soc_enabled', 'b'],
  ['auto_calibrate_soc_drift_percent', 'u8'],
  ['', ' '],
  ['autocal_dwell_accumulated_ms', 'u32'],
  ['autocal_grace_timer_ms', 'u32'],
  ['autocal_drift_percent', 'f'],
  ['autocal_current_dA', 'i16'],
  ['autocal_crit_taper', 'b'],
  ['autocal_crit_low_current', 'b'],
  ['autocal_crit_dwell', 'b'],
  ['autocal_crit_drift', 'b'],
  ['autocal_crit_cooldown_ready', 'b'],
  ['autocal_crit_contactors', 'b'],
  ['dtc_read_in_progress', 'b'],
  ['UserRequestDTCreadout', 'b'],
  ['UserRequestDTCreset', 'b'],
  ['UserRequestIsoRoutineEnable', 'b'],
  ['UserRequestIsoRoutineDisable', 'b'],
  ['keep_iso_disabled', 'b'],
  ['iso_measurement_active', 'b'],
  ['iso_status_valid', 'b'],
  ['iso_command_status', 'u8'],
];

export const SOC_estimated: [number, string] = [0, 'u16'];
export const SOC_highprec: [number, string] = [2, 'u16'];
export const SOC_polled: [number, string] = [4, 'u16'];
export const pack_voltage_dV: [number, string] = [6, 'u16'];
export const insulation_ohm_per_volt: [number, string] = [8, 'u16'];
export const insulation_valid: [number, string] = [10, 'b'];
export const chargePower: [number, string] = [12, 'u16'];
export const charge_times: [number, string] = [14, 'u16'];
export const dischargePower: [number, string] = [16, 'u16'];
export const total_charged_ah: [number, string] = [18, 'u16'];
export const total_discharged_ah: [number, string] = [20, 'u16'];
export const total_charged_kwh: [number, string] = [22, 'u16'];
export const total_discharged_kwh: [number, string] = [24, 'u16'];
export const times_full_power: [number, string] = [26, 'u16'];
export const BMS_capacity_original_calibration: [number, string] = [28, 'u16'];
export const BMC_SOC_original_calibration: [number, string] = [30, 'u16'];
export const BMS_capacity_current_calibration: [number, string] = [32, 'u16'];
export const BMC_SOC_current_calibration: [number, string] = [34, 'u16'];
export const seed: [number, string] = [36, 'u16'];
export const solvedKey: [number, string] = [38, 'u16'];
export const calibrationTargetSOC: [number, string] = [40, 'u16'];
export const calibrationTargetAH: [number, string] = [42, 'u16'];
export const battery_temperatures: [number, string, number] = [44, 'i16', 13];
export const discharge_status: [number, string] = [70, 'u8'];
export const BMS_min_cell_voltage_number: [number, string] = [71, 'u8'];
export const BMS_min_temp_module_number: [number, string] = [72, 'u8'];
export const BMS_max_cell_voltage_number: [number, string] = [73, 'u8'];
export const BMS_max_temp_module_number: [number, string] = [74, 'u8'];
export const servicemode: [number, string] = [75, 'u8'];
export const UserRequestCrashReset: [number, string] = [76, 'b'];
export const UserRequestCalibrateSOC: [number, string] = [77, 'b'];
export const contactor_control_state: [number, string] = [78, 'u8'];
export const contactor_feedback: [number, string] = [79, 'u8'];
export const contactor_main_closed: [number, string] = [80, 'b'];
export const contactor_precharging: [number, string] = [81, 'b'];
export const contactor_hv_active: [number, string] = [82, 'b'];
export const contactor_drive_flag: [number, string] = [83, 'b'];
export const contactor_charge_flag: [number, string] = [84, 'b'];
export const auto_calibrate_soc_enabled: [number, string] = [85, 'b'];
export const auto_calibrate_soc_drift_percent: [number, string] = [86, 'u8'];
export const autocal_dwell_accumulated_ms: [number, string] = [88, 'u32'];
export const autocal_grace_timer_ms: [number, string] = [92, 'u32'];
export const autocal_drift_percent: [number, string] = [96, 'f'];
export const autocal_current_dA: [number, string] = [100, 'i16'];
export const autocal_crit_taper: [number, string] = [102, 'b'];
export const autocal_crit_low_current: [number, string] = [103, 'b'];
export const autocal_crit_dwell: [number, string] = [104, 'b'];
export const autocal_crit_drift: [number, string] = [105, 'b'];
export const autocal_crit_cooldown_ready: [number, string] = [106, 'b'];
export const autocal_crit_contactors: [number, string] = [107, 'b'];
export const dtc_read_in_progress: [number, string] = [108, 'b'];
export const UserRequestDTCreadout: [number, string] = [109, 'b'];
export const UserRequestDTCreset: [number, string] = [110, 'b'];
export const UserRequestIsoRoutineEnable: [number, string] = [111, 'b'];
export const UserRequestIsoRoutineDisable: [number, string] = [112, 'b'];
export const keep_iso_disabled: [number, string] = [113, 'b'];
export const iso_measurement_active: [number, string] = [114, 'b'];
export const iso_status_valid: [number, string] = [115, 'b'];
export const iso_command_status: [number, string] = [116, 'u8'];
