// BydAttoBattery: 1360 bytes; base classes: CanBattery@0
export const BYD_ATTO_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['renderer', 'BydAtto3HtmlRenderer'],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['datalayer_bydatto', 'DATALAYER_INFO_BYDATTO3*'],
  ['allows_contactor_closing', 'bool*'],
  ['previousMillis50', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis200', 'u32'],
  ['last_auto_calibrate_ms', 'u64'],
  ['autocal_dwell_ms', 'u32'],
  ['autocal_grace_start_ms', 'u32'],
  ['rampdown_power', 'u16'],
  ['poll_state', 'u16'],
  ['pid_reply', 'u16'],
  ['battery_voltage', 'u16'],
  ['battery_voltage_dV', 'u16'],
  ['battery_insulation_ohm_per_volt', 'u16'],
  ['battery_highprecision_SOC', 'u16'],
  ['battery_estimated_SOC', 'u16'],
  ['BMS_SOC', 'u16'],
  ['BMS_lowest_cell_voltage_mV', 'u16'],
  ['BMS_highest_cell_voltage_mV', 'u16'],
  ['BMS_allowed_charge_power', 'u16'],
  ['BMS_charge_times', 'u16'],
  ['BMS_allowed_discharge_power', 'u16'],
  ['BMS_total_charged_ah', 'u16'],
  ['BMS_total_discharged_ah', 'u16'],
  ['BMS_total_charged_kwh', 'u16'],
  ['BMS_total_discharged_kwh', 'u16'],
  ['BMS_times_full_power', 'u16'],
  ['BMS_capacity_original_calibration', 'u16'],
  ['BMC_SOC_original_calibration', 'u16'],
  ['BMS_capacity_current_calibration', 'u16'],
  ['BMC_SOC_current_calibration', 'u16'],
  ['seed', 'u16'],
  ['solvedKey', 'u16'],
  ['battery_temperature_ambient', 'i16'],
  ['battery_calc_min_temperature', 'i16'],
  ['battery_calc_max_temperature', 'i16'],
  ['battery_current_dA', 'i16'],
  ['BMS_lowest_cell_temperature', 'i16'],
  ['BMS_highest_cell_temperature', 'i16'],
  ['BMS_average_cell_temperature', 'i16'],
  ['battery_type', 'u8'],
  ['stateMachineClearCrash', 'u8'],
  ['stateMachineCalibrateSOC', 'u8'],
  ['stateMachineIsoRoutine', 'u8'],
  ['isoRoutineAction', 'u8'],
  ['increaseTimeoutIso', 'u8'],
  ['bms_was_alive', 'b'],
  ['iso_reassert_needed', 'b'],
  ['bms_alive_since_ms', 'u32'],
  ['iso_reassert_attempt_ms', 'u32'],
  ['stateMachineReadDTC', 'u8'],
  ['stateMachineEraseDTC', 'u8'],
  ['', ' ', 2],
  ['dtc_request_millis', 'u32'],
  ['dtc_buffer', 'u8', 140],
  ['dtc_rx_expected', 'u16'],
  ['dtc_rx_len', 'u16'],
  ['dtc_rx_active', 'b'],
  ['contactorState', 'u8'],
  ['contactor_feedback', 'u8'],
  ['', ' '],
  ['contactorStateEntryMillis', 'u32'],
  ['closeConfirmStartMillis', 'u32'],
  ['lastCurrentSampleMillis', 'u32'],
  ['lastContactorFeedbackMillis', 'u32'],
  ['closeConfirmPending', 'b'],
  ['openTimeoutEventSent', 'b'],
  ['requestContactorOpen', 'b'],
  ['requestContactorClose', 'b'],
  ['previousContactorsAllowedClosed', 'b'],
  ['contactorControlInitialized', 'b'],
  ['counter_50ms', 'u8'],
  ['counter_100ms', 'u8'],
  ['frame6_counter', 'u8'],
  ['BMS_SOH', 'u8'],
  ['BMS_min_cell_voltage_number', 'u8'],
  ['BMS_min_temp_module_number', 'u8'],
  ['BMS_max_cell_voltage_number', 'u8'],
  ['BMS_max_temp_module_number', 'u8'],
  ['battery_frame_index', 'u8'],
  ['discharge_status', 'u8'],
  ['increaseTimeoutSOC', 'u8'],
  ['servicemode', 'u8'],
  ['secondsSinceStartup', 'u8'],
  ['BMS_voltage_available', 'b'],
  ['battery_insulation_valid', 'b'],
  ['battery_iso_measurement_active', 'b'],
  ['', ' ', 2],
  ['last_35E_ms', 'u32'],
  ['calibrationAH_seeded', 'b'],
  ['', ' '],
  ['battery_daughterboard_temperatures', 'i16', 13],
  ['battery_cellvoltages', 'u16', 192],
  ['', ' ', 4],
  ['ATTO_3_12D', 'CAN_frame'],
  ['ATTO_3_441', 'CAN_frame'],
  ['ATTO_3_7E7_POLL', 'CAN_frame'],
  ['ATTO_3_7E7_ACK', 'CAN_frame'],
  ['ATTO_3_7E7_CLEAR_CRASH', 'CAN_frame'],
  ['ATTO_3_7E7_RESET_SOC', 'CAN_frame'],
  ['ATTO_3_7E7_READ_DTC', 'CAN_frame'],
  ['ATTO_3_7E7_DTC_FC', 'CAN_frame'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const renderer: [number, string] = [24, 'BydAtto3HtmlRenderer'];
export const datalayer_battery: [number, string] = [48, 'DATALAYER_BATTERY_TYPE*'];
export const datalayer_bydatto: [number, string] = [52, 'DATALAYER_INFO_BYDATTO3*'];
export const allows_contactor_closing: [number, string] = [56, 'bool*'];
export const previousMillis50: [number, string] = [60, 'u32'];
export const previousMillis100: [number, string] = [64, 'u32'];
export const previousMillis200: [number, string] = [68, 'u32'];
export const last_auto_calibrate_ms: [number, string] = [72, 'u64'];
export const autocal_dwell_ms: [number, string] = [80, 'u32'];
export const autocal_grace_start_ms: [number, string] = [84, 'u32'];
export const rampdown_power: [number, string] = [88, 'u16'];
export const poll_state: [number, string] = [90, 'u16'];
export const pid_reply: [number, string] = [92, 'u16'];
export const battery_voltage: [number, string] = [94, 'u16'];
export const battery_voltage_dV: [number, string] = [96, 'u16'];
export const battery_insulation_ohm_per_volt: [number, string] = [98, 'u16'];
export const battery_highprecision_SOC: [number, string] = [100, 'u16'];
export const battery_estimated_SOC: [number, string] = [102, 'u16'];
export const BMS_SOC: [number, string] = [104, 'u16'];
export const BMS_lowest_cell_voltage_mV: [number, string] = [106, 'u16'];
export const BMS_highest_cell_voltage_mV: [number, string] = [108, 'u16'];
export const BMS_allowed_charge_power: [number, string] = [110, 'u16'];
export const BMS_charge_times: [number, string] = [112, 'u16'];
export const BMS_allowed_discharge_power: [number, string] = [114, 'u16'];
export const BMS_total_charged_ah: [number, string] = [116, 'u16'];
export const BMS_total_discharged_ah: [number, string] = [118, 'u16'];
export const BMS_total_charged_kwh: [number, string] = [120, 'u16'];
export const BMS_total_discharged_kwh: [number, string] = [122, 'u16'];
export const BMS_times_full_power: [number, string] = [124, 'u16'];
export const BMS_capacity_original_calibration: [number, string] = [126, 'u16'];
export const BMC_SOC_original_calibration: [number, string] = [128, 'u16'];
export const BMS_capacity_current_calibration: [number, string] = [130, 'u16'];
export const BMC_SOC_current_calibration: [number, string] = [132, 'u16'];
export const seed: [number, string] = [134, 'u16'];
export const solvedKey: [number, string] = [136, 'u16'];
export const battery_temperature_ambient: [number, string] = [138, 'i16'];
export const battery_calc_min_temperature: [number, string] = [140, 'i16'];
export const battery_calc_max_temperature: [number, string] = [142, 'i16'];
export const battery_current_dA: [number, string] = [144, 'i16'];
export const BMS_lowest_cell_temperature: [number, string] = [146, 'i16'];
export const BMS_highest_cell_temperature: [number, string] = [148, 'i16'];
export const BMS_average_cell_temperature: [number, string] = [150, 'i16'];
export const battery_type: [number, string] = [152, 'u8'];
export const stateMachineClearCrash: [number, string] = [153, 'u8'];
export const stateMachineCalibrateSOC: [number, string] = [154, 'u8'];
export const stateMachineIsoRoutine: [number, string] = [155, 'u8'];
export const isoRoutineAction: [number, string] = [156, 'u8'];
export const increaseTimeoutIso: [number, string] = [157, 'u8'];
export const bms_was_alive: [number, string] = [158, 'b'];
export const iso_reassert_needed: [number, string] = [159, 'b'];
export const bms_alive_since_ms: [number, string] = [160, 'u32'];
export const iso_reassert_attempt_ms: [number, string] = [164, 'u32'];
export const stateMachineReadDTC: [number, string] = [168, 'u8'];
export const stateMachineEraseDTC: [number, string] = [169, 'u8'];
export const dtc_request_millis: [number, string] = [172, 'u32'];
export const dtc_buffer: [number, string, number] = [176, 'u8', 140];
export const dtc_rx_expected: [number, string] = [316, 'u16'];
export const dtc_rx_len: [number, string] = [318, 'u16'];
export const dtc_rx_active: [number, string] = [320, 'b'];
export const contactorState: [number, string] = [321, 'u8'];
export const contactor_feedback: [number, string] = [322, 'u8'];
export const contactorStateEntryMillis: [number, string] = [324, 'u32'];
export const closeConfirmStartMillis: [number, string] = [328, 'u32'];
export const lastCurrentSampleMillis: [number, string] = [332, 'u32'];
export const lastContactorFeedbackMillis: [number, string] = [336, 'u32'];
export const closeConfirmPending: [number, string] = [340, 'b'];
export const openTimeoutEventSent: [number, string] = [341, 'b'];
export const requestContactorOpen: [number, string] = [342, 'b'];
export const requestContactorClose: [number, string] = [343, 'b'];
export const previousContactorsAllowedClosed: [number, string] = [344, 'b'];
export const contactorControlInitialized: [number, string] = [345, 'b'];
export const counter_50ms: [number, string] = [346, 'u8'];
export const counter_100ms: [number, string] = [347, 'u8'];
export const frame6_counter: [number, string] = [348, 'u8'];
export const BMS_SOH: [number, string] = [349, 'u8'];
export const BMS_min_cell_voltage_number: [number, string] = [350, 'u8'];
export const BMS_min_temp_module_number: [number, string] = [351, 'u8'];
export const BMS_max_cell_voltage_number: [number, string] = [352, 'u8'];
export const BMS_max_temp_module_number: [number, string] = [353, 'u8'];
export const battery_frame_index: [number, string] = [354, 'u8'];
export const discharge_status: [number, string] = [355, 'u8'];
export const increaseTimeoutSOC: [number, string] = [356, 'u8'];
export const servicemode: [number, string] = [357, 'u8'];
export const secondsSinceStartup: [number, string] = [358, 'u8'];
export const BMS_voltage_available: [number, string] = [359, 'b'];
export const battery_insulation_valid: [number, string] = [360, 'b'];
export const battery_iso_measurement_active: [number, string] = [361, 'b'];
export const last_35E_ms: [number, string] = [364, 'u32'];
export const calibrationAH_seeded: [number, string] = [368, 'b'];
export const battery_daughterboard_temperatures: [number, string, number] = [370, 'i16', 13];
export const battery_cellvoltages: [number, string, number] = [396, 'u16', 192];
export const ATTO_3_12D: [number, string] = [784, 'CAN_frame'];
export const ATTO_3_441: [number, string] = [856, 'CAN_frame'];
export const ATTO_3_7E7_POLL: [number, string] = [928, 'CAN_frame'];
export const ATTO_3_7E7_ACK: [number, string] = [1000, 'CAN_frame'];
export const ATTO_3_7E7_CLEAR_CRASH: [number, string] = [1072, 'CAN_frame'];
export const ATTO_3_7E7_RESET_SOC: [number, string] = [1144, 'CAN_frame'];
export const ATTO_3_7E7_READ_DTC: [number, string] = [1216, 'CAN_frame'];
export const ATTO_3_7E7_DTC_FC: [number, string] = [1288, 'CAN_frame'];
