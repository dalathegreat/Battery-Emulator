// BmwPhevBattery: 3176 bytes; base classes: CanBattery@0
export const BMW_PHEV_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['UDS_inProgress', 'b'],
  ['previous', 'b'],
  ['present', 'b'],
  ['UDS_expectedLength', 'u16'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['UDS_bytesReceived', 'u16'],
  ['UDS_moduleID', 'u8'],
  ['receivedInBatch', 'u8'],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['UDS_buffer', 'u8', 256],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['renderer', 'BmwPhevHtmlRenderer'],
  ['previousMillis20', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis200', 'u32'],
  ['previousMillis500', 'u32'],
  ['previousMillis640', 'u32'],
  ['previousMillis1000', 'u32'],
  ['previousMillis5000', 'u32'],
  ['previousMillis10000', 'u32'],
  ['min_cell_voltage_lastchanged', 'u32'],
  ['max_cell_voltage_lastchanged', 'u32'],
  ['cmdState', 'CmdState'],
  ['gUDSContext', 'UDS_RxContext'],
  ['UDS_lastFrameMillis', 'u32'],
  ['uds_one_shot_sent_ms', 'u32'],
  ['BMW_12F', 'CAN_frame'],
  ['BMW_10B', 'CAN_frame'],
  ['BMW_53A', 'CAN_frame'],
  ['BMW_1A1', 'CAN_frame'],
  ['BMW_3A0', 'CAN_frame'],
  ['BMW_328', 'CAN_frame'],
  ['BMW_3CA', 'CAN_frame'],
  ['BMW_433', 'CAN_frame'],
  ['BMW_2CA', 'CAN_frame'],
  ['BMW_3E8', 'CAN_frame'],
  ['BMW_PHEV_BUS_WAKEUP_REQUEST', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_SOC', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_SOH', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_CURRENT', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_VOLTAGE_LIMITS', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_READ_DTC', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_CLEAR_DTC', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_PAIRED_VIN', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_PACK_INFO', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_CURRENT_LIMITS', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_MAINVOLTAGE_PRECONTACTOR', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_MAINVOLTAGE_POSTCONTACTOR', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_CELLSUMMARY', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_CELLS_INDIVIDUAL_VOLTS', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_CELL_TEMP', 'CAN_frame'],
  ['BMW_6F1_REQUEST_CONTINUE_MULTIFRAME', 'CAN_frame'],
  ['BMW_6F1_REQUEST_HARD_RESET', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_CONTACTORS_CLOSE', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_CONTACTORS_OPEN', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_BALANCING_STATUS', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_ISOLATION_TEST', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_ISOLATION_RESULT', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_ISO_READING1', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_ISO_READING2', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_BALANCING_START', 'CAN_frame'],
  ['BMWPHEV_6F1_REQUEST_BALANCING_STOP', 'CAN_frame'],
  ['UDS_REQUESTS_FAST', 'a53e'],
  ['numFastUDSreqs', 'i32'],
  ['UDS_REQUESTS_SLOW', 'a54e'],
  ['numSlowUDSreqs', 'i32'],
  ['battery_BEV_available_power_shortterm_charge', 'u32'],
  ['battery_BEV_available_power_shortterm_discharge', 'u32'],
  ['battery_BEV_available_power_longterm_charge', 'u32'],
  ['battery_BEV_available_power_longterm_discharge', 'u32'],
  ['battery_current', 'i32'],
  ['battery_max_charge_voltage', 'u16'],
  ['battery_min_discharge_voltage', 'u16'],
  ['battery_predicted_energy_charge_condition', 'u16'],
  ['battery_predicted_energy_charging_target', 'u16'],
  ['battery_prediction_voltage_shortterm_charge', 'u16'],
  ['battery_prediction_voltage_shortterm_discharge', 'u16'],
  ['battery_prediction_voltage_longterm_charge', 'u16'],
  ['battery_prediction_voltage_longterm_discharge', 'u16'],
  ['battery_prediction_duration_charging_minutes', 'u16'],
  ['battery_energy_content_maximum_kWh', 'u16'],
  ['battery_target_voltage_in_CV_mode', 'u16'],
  ['battery_display_SOC', 'u16'],
  ['battery_voltage', 'u16'],
  ['battery_voltage_after_contactor', 'u16'],
  ['avg_soc_state', 'u16'],
  ['min_soh_state', 'u16'],
  ['max_design_voltage', 'u16'],
  ['min_design_voltage', 'u16'],
  ['iso_safety_int_kohm', 'u16'],
  ['iso_safety_ext_kohm', 'u16'],
  ['iso_safety_trg_kohm', 'u16'],
  ['iso_safety_kohm', 'u16'],
  ['battery_max_discharge_amperage', 'i16'],
  ['battery_max_charge_amperage', 'i16'],
  ['battery_temperature_max', 'i16'],
  ['battery_temperature_min', 'i16'],
  ['min_cell_voltage', 'i16'],
  ['max_cell_voltage', 'i16'],
  ['allowable_charge_amps', 'i16'],
  ['allowable_discharge_amps', 'i16'],
  ['battery_status_service_disconnection_plug', 'u8'],
  ['battery_status_measurement_isolation', 'u8'],
  ['battery_request_abort_charging', 'u8'],
  ['battery_prediction_time_end_of_charging_minutes', 'u8'],
  ['battery_request_charging_condition_minimum', 'u8'],
  ['battery_request_charging_condition_maximum', 'u8'],
  ['battery_request_operating_mode', 'u8'],
  ['battery_status_error_isolation_external_Bordnetz', 'u8'],
  ['battery_status_error_isolation_internal_Bordnetz', 'u8'],
  ['battery_request_cooling', 'u8'],
  ['battery_status_valve_cooling', 'u8'],
  ['battery_status_error_locking', 'u8'],
  ['battery_status_precharge_locked', 'u8'],
  ['battery_status_disconnecting_switch', 'u8'],
  ['battery_status_emergency_mode', 'u8'],
  ['battery_request_service', 'u8'],
  ['battery_error_emergency_mode', 'u8'],
  ['battery_status_error_disconnecting_switch', 'u8'],
  ['battery_status_warning_isolation', 'u8'],
  ['battery_status_cold_shutoff_valve', 'u8'],
  ['battery_request_open_contactors', 'u8'],
  ['battery_request_open_contactors_instantly', 'u8'],
  ['battery_request_open_contactors_fast', 'u8'],
  ['battery_charging_condition_delta', 'u8'],
  ['startup_counter_contactor', 'u8'],
  ['alive_counter_20ms', 'u8'],
  ['iso_safety_ext_plausible', 'u8'],
  ['iso_safety_int_plausible', 'u8'],
  ['iso_safety_trg_plausible', 'u8'],
  ['iso_safety_kohm_quality', 'u8'],
  ['balancing_status', 'u8'],
  ['phev_balancing_stop_sent', 'b'],
  ['uds_fast_req_id_counter', 'u8'],
  ['uds_slow_req_id_counter', 'u8'],
  ['alive_counter_100ms', 'u8'],
  ['BMW_328_seconds', 'u32'],
  ['BMW_328_days', 'u16'],
  ['BMW_328_seconds_to_day', 'u32'],
  ['paired_vin', 'u8', 17],
  ['pack_limit_info_available', 'b'],
  ['battery_awake', 'b'],
  ['userRequestContactorClose', 'b'],
  ['userRequestContactorOpen', 'b'],
  ['phev_last_equipment_stop', 'b'],
  ['contactorCloseReq', 'b'],
  ['contactor_close_start_ms', 'u32'],
  ['phev_pre_close_stops_remaining', 'u8'],
  ['phev_pre_close_stop_last_ms', 'u32'],
  ['phev_last_balancing_request', 'b'],
  ['phev_balancing_burst_start', 'b'],
  ['phev_balancing_bursts_remaining', 'u8'],
  ['phev_balancing_burst_last_ms', 'u32'],
  ['phev_53a_state', 'Phev53AState'],
  ['InverterContactorCloseRequest', 'InverterContactorCloseRequestStruct'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const UDS_inProgress: [number, string] = [0, 'b'];
export const previous: [number, string] = [0, 'b'];
export const present: [number, string] = [1, 'b'];
export const UDS_expectedLength: [number, string] = [2, 'u16'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const UDS_bytesReceived: [number, string] = [4, 'u16'];
export const UDS_moduleID: [number, string] = [6, 'u8'];
export const receivedInBatch: [number, string] = [7, 'u8'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const UDS_buffer: [number, string, number] = [8, 'u8', 256];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const renderer: [number, string] = [24, 'BmwPhevHtmlRenderer'];
export const previousMillis20: [number, string] = [28, 'u32'];
export const previousMillis100: [number, string] = [32, 'u32'];
export const previousMillis200: [number, string] = [36, 'u32'];
export const previousMillis500: [number, string] = [40, 'u32'];
export const previousMillis640: [number, string] = [44, 'u32'];
export const previousMillis1000: [number, string] = [48, 'u32'];
export const previousMillis5000: [number, string] = [52, 'u32'];
export const previousMillis10000: [number, string] = [56, 'u32'];
export const min_cell_voltage_lastchanged: [number, string] = [60, 'u32'];
export const max_cell_voltage_lastchanged: [number, string] = [64, 'u32'];
export const cmdState: [number, string] = [68, 'CmdState'];
export const gUDSContext: [number, string] = [72, 'UDS_RxContext'];
export const UDS_lastFrameMillis: [number, string] = [264, 'u32'];
export const uds_one_shot_sent_ms: [number, string] = [340, 'u32'];
export const BMW_12F: [number, string] = [344, 'CAN_frame'];
export const BMW_10B: [number, string] = [416, 'CAN_frame'];
export const BMW_53A: [number, string] = [488, 'CAN_frame'];
export const BMW_1A1: [number, string] = [560, 'CAN_frame'];
export const BMW_3A0: [number, string] = [632, 'CAN_frame'];
export const BMW_328: [number, string] = [704, 'CAN_frame'];
export const BMW_3CA: [number, string] = [776, 'CAN_frame'];
export const BMW_433: [number, string] = [848, 'CAN_frame'];
export const BMW_2CA: [number, string] = [920, 'CAN_frame'];
export const BMW_3E8: [number, string] = [992, 'CAN_frame'];
export const BMW_PHEV_BUS_WAKEUP_REQUEST: [number, string] = [1064, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_SOC: [number, string] = [1136, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_SOH: [number, string] = [1208, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_CURRENT: [number, string] = [1280, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_VOLTAGE_LIMITS: [number, string] = [1352, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_READ_DTC: [number, string] = [1424, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_CLEAR_DTC: [number, string] = [1496, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_PAIRED_VIN: [number, string] = [1568, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_PACK_INFO: [number, string] = [1640, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_CURRENT_LIMITS: [number, string] = [1712, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_MAINVOLTAGE_PRECONTACTOR: [number, string] = [1784, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_MAINVOLTAGE_POSTCONTACTOR: [number, string] = [1856, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_CELLSUMMARY: [number, string] = [1928, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_CELLS_INDIVIDUAL_VOLTS: [number, string] = [2000, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_CELL_TEMP: [number, string] = [2072, 'CAN_frame'];
export const BMW_6F1_REQUEST_CONTINUE_MULTIFRAME: [number, string] = [2144, 'CAN_frame'];
export const BMW_6F1_REQUEST_HARD_RESET: [number, string] = [2216, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_CONTACTORS_CLOSE: [number, string] = [2288, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_CONTACTORS_OPEN: [number, string] = [2360, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_BALANCING_STATUS: [number, string] = [2432, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_ISOLATION_TEST: [number, string] = [2504, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_ISOLATION_RESULT: [number, string] = [2576, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_ISO_READING1: [number, string] = [2648, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_ISO_READING2: [number, string] = [2720, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_BALANCING_START: [number, string] = [2792, 'CAN_frame'];
export const BMWPHEV_6F1_REQUEST_BALANCING_STOP: [number, string] = [2864, 'CAN_frame'];
export const UDS_REQUESTS_FAST: [number, string] = [2936, 'a53e'];
export const numFastUDSreqs: [number, string] = [2952, 'i32'];
export const UDS_REQUESTS_SLOW: [number, string] = [2956, 'a54e'];
export const numSlowUDSreqs: [number, string] = [2992, 'i32'];
export const battery_BEV_available_power_shortterm_charge: [number, string] = [2996, 'u32'];
export const battery_BEV_available_power_shortterm_discharge: [number, string] = [3000, 'u32'];
export const battery_BEV_available_power_longterm_charge: [number, string] = [3004, 'u32'];
export const battery_BEV_available_power_longterm_discharge: [number, string] = [3008, 'u32'];
export const battery_current: [number, string] = [3012, 'i32'];
export const battery_max_charge_voltage: [number, string] = [3016, 'u16'];
export const battery_min_discharge_voltage: [number, string] = [3018, 'u16'];
export const battery_predicted_energy_charge_condition: [number, string] = [3020, 'u16'];
export const battery_predicted_energy_charging_target: [number, string] = [3022, 'u16'];
export const battery_prediction_voltage_shortterm_charge: [number, string] = [3024, 'u16'];
export const battery_prediction_voltage_shortterm_discharge: [number, string] = [3026, 'u16'];
export const battery_prediction_voltage_longterm_charge: [number, string] = [3028, 'u16'];
export const battery_prediction_voltage_longterm_discharge: [number, string] = [3030, 'u16'];
export const battery_prediction_duration_charging_minutes: [number, string] = [3032, 'u16'];
export const battery_energy_content_maximum_kWh: [number, string] = [3034, 'u16'];
export const battery_target_voltage_in_CV_mode: [number, string] = [3036, 'u16'];
export const battery_display_SOC: [number, string] = [3038, 'u16'];
export const battery_voltage: [number, string] = [3040, 'u16'];
export const battery_voltage_after_contactor: [number, string] = [3042, 'u16'];
export const avg_soc_state: [number, string] = [3044, 'u16'];
export const min_soh_state: [number, string] = [3046, 'u16'];
export const max_design_voltage: [number, string] = [3048, 'u16'];
export const min_design_voltage: [number, string] = [3050, 'u16'];
export const iso_safety_int_kohm: [number, string] = [3052, 'u16'];
export const iso_safety_ext_kohm: [number, string] = [3054, 'u16'];
export const iso_safety_trg_kohm: [number, string] = [3056, 'u16'];
export const iso_safety_kohm: [number, string] = [3058, 'u16'];
export const battery_max_discharge_amperage: [number, string] = [3060, 'i16'];
export const battery_max_charge_amperage: [number, string] = [3062, 'i16'];
export const battery_temperature_max: [number, string] = [3064, 'i16'];
export const battery_temperature_min: [number, string] = [3066, 'i16'];
export const min_cell_voltage: [number, string] = [3068, 'i16'];
export const max_cell_voltage: [number, string] = [3070, 'i16'];
export const allowable_charge_amps: [number, string] = [3072, 'i16'];
export const allowable_discharge_amps: [number, string] = [3074, 'i16'];
export const battery_status_service_disconnection_plug: [number, string] = [3076, 'u8'];
export const battery_status_measurement_isolation: [number, string] = [3077, 'u8'];
export const battery_request_abort_charging: [number, string] = [3078, 'u8'];
export const battery_prediction_time_end_of_charging_minutes: [number, string] = [3079, 'u8'];
export const battery_request_charging_condition_minimum: [number, string] = [3080, 'u8'];
export const battery_request_charging_condition_maximum: [number, string] = [3081, 'u8'];
export const battery_request_operating_mode: [number, string] = [3082, 'u8'];
export const battery_status_error_isolation_external_Bordnetz: [number, string] = [3083, 'u8'];
export const battery_status_error_isolation_internal_Bordnetz: [number, string] = [3084, 'u8'];
export const battery_request_cooling: [number, string] = [3085, 'u8'];
export const battery_status_valve_cooling: [number, string] = [3086, 'u8'];
export const battery_status_error_locking: [number, string] = [3087, 'u8'];
export const battery_status_precharge_locked: [number, string] = [3088, 'u8'];
export const battery_status_disconnecting_switch: [number, string] = [3089, 'u8'];
export const battery_status_emergency_mode: [number, string] = [3090, 'u8'];
export const battery_request_service: [number, string] = [3091, 'u8'];
export const battery_error_emergency_mode: [number, string] = [3092, 'u8'];
export const battery_status_error_disconnecting_switch: [number, string] = [3093, 'u8'];
export const battery_status_warning_isolation: [number, string] = [3094, 'u8'];
export const battery_status_cold_shutoff_valve: [number, string] = [3095, 'u8'];
export const battery_request_open_contactors: [number, string] = [3096, 'u8'];
export const battery_request_open_contactors_instantly: [number, string] = [3097, 'u8'];
export const battery_request_open_contactors_fast: [number, string] = [3098, 'u8'];
export const battery_charging_condition_delta: [number, string] = [3099, 'u8'];
export const startup_counter_contactor: [number, string] = [3100, 'u8'];
export const alive_counter_20ms: [number, string] = [3101, 'u8'];
export const iso_safety_ext_plausible: [number, string] = [3102, 'u8'];
export const iso_safety_int_plausible: [number, string] = [3103, 'u8'];
export const iso_safety_trg_plausible: [number, string] = [3104, 'u8'];
export const iso_safety_kohm_quality: [number, string] = [3105, 'u8'];
export const balancing_status: [number, string] = [3106, 'u8'];
export const phev_balancing_stop_sent: [number, string] = [3107, 'b'];
export const uds_fast_req_id_counter: [number, string] = [3108, 'u8'];
export const uds_slow_req_id_counter: [number, string] = [3109, 'u8'];
export const alive_counter_100ms: [number, string] = [3110, 'u8'];
export const BMW_328_seconds: [number, string] = [3112, 'u32'];
export const BMW_328_days: [number, string] = [3116, 'u16'];
export const BMW_328_seconds_to_day: [number, string] = [3120, 'u32'];
export const paired_vin: [number, string, number] = [3124, 'u8', 17];
export const pack_limit_info_available: [number, string] = [3141, 'b'];
export const battery_awake: [number, string] = [3142, 'b'];
export const userRequestContactorClose: [number, string] = [3143, 'b'];
export const userRequestContactorOpen: [number, string] = [3144, 'b'];
export const phev_last_equipment_stop: [number, string] = [3145, 'b'];
export const contactorCloseReq: [number, string] = [3146, 'b'];
export const contactor_close_start_ms: [number, string] = [3148, 'u32'];
export const phev_pre_close_stops_remaining: [number, string] = [3152, 'u8'];
export const phev_pre_close_stop_last_ms: [number, string] = [3156, 'u32'];
export const phev_last_balancing_request: [number, string] = [3160, 'b'];
export const phev_balancing_burst_start: [number, string] = [3161, 'b'];
export const phev_balancing_bursts_remaining: [number, string] = [3162, 'u8'];
export const phev_balancing_burst_last_ms: [number, string] = [3164, 'u32'];
export const phev_53a_state: [number, string] = [3168, 'Phev53AState'];
export const InverterContactorCloseRequest: [number, string] = [3172, 'InverterContactorCloseRequestStruct'];
