// FordMachEBattery: 1336 bytes; base classes: CanBattery@0
export const FORD_MACH_EBATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['renderer', 'FordMachEHtmlRenderer'],
  ['', ' ', 8],
  ['UserRequestDTCreset', 'b'],
  ['UserRequestDTCreadout', 'b'],
  ['', ' ', 2],
  ['previousMillis20', 'u32'],
  ['previousMillis30', 'u32'],
  ['previousMillis50', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis250', 'u32'],
  ['previousMillis1000', 'u32'],
  ['previousMillis10s', 'u32'],
  ['cell_temperature', 'i16', 6],
  ['maximum_temperature', 'i16'],
  ['minimum_temperature', 'i16'],
  ['battery_soc', 'u16'],
  ['battery_soh', 'u16'],
  ['battery_voltage', 'u16'],
  ['battery_current', 'i16'],
  ['maximum_cellvoltage_mV', 'u16'],
  ['minimum_cellvoltage_mV', 'u16'],
  ['charge_current_allowed', 'u16'],
  ['discharge_current_allowed', 'u16'],
  ['pid_reply', 'u16'],
  ['polled_12V', 'u16'],
  ['display_soc', 'u8'],
  ['', ' '],
  ['currentpoll', 'u16'],
  ['poll_index', 'u8'],
  ['', ' '],
  ['poll_commands', 'a0e1'],
  ['', ' ', 72],
  ['pid_hvb_temp', 'i16'],
  ['pid_hvb_soc', 'u32'],
  ['pid_hvb_contactor_status', 'u32'],
  ['pid_hvb_contactor_positive_leak_voltage', 'u16'],
  ['pid_hvb_contactor_negative_leak_voltage', 'u16'],
  ['pid_hvb_contactor_positive_voltage', 'u16'],
  ['pid_hvb_contactor_negative_voltage', 'u16'],
  ['pid_hvb_contactor_positive_bus_leak_resistance', 'u16'],
  ['pid_hvb_contactor_negative_bus_leak_resistance', 'u16'],
  ['pid_hvb_contactor_overall_leak_resistance', 'u16'],
  ['pid_hvb_contactor_open_leak_resistance', 'u16'],
  ['pid_hvb_ete', 'u16'],
  ['pid_hvb_current', 'u16'],
  ['pid_charger_power_limit', 'u16'],
  ['pid_hvb_soh', 'u8'],
  ['', ' '],
  ['pid_hvb_voltage', 'u16'],
  ['pid_hvb_max_charge_current', 'u16'],
  ['pid_hvb_charge_voltage_requested', 'u16'],
  ['pid_hvb_soc_d', 'u16'],
  ['pid_hvb_charge_current_requested', 'u16'],
  ['pid_gear_commanded', 'u8'],
  ['pid_key_state', 'u8'],
  ['pid_charge_plug', 'u8'],
  ['pid_charger_output_voltage', 'u8'],
  ['pid_charger_status', 'u8'],
  ['pid_charger_output_current_measured', 'u8'],
  ['pid_evse_type', 'u8'],
  ['pid_charger_max_power', 'u8'],
  ['pid_charging_status', 'u8'],
  ['pid_charger_input_power_available', 'u8'],
  ['pid_time', 'u8'],
  ['pid_lores_odometer', 'u8'],
  ['pid_engine_runtime', 'u8'],
  ['', ' '],
  ['pid_hvb_calendar_age_months', 'u16'],
  ['pid_battery_capacity_ah', 'u16'],
  ['pid_maintenance_rebalance_status', 'u8'],
  ['', ' '],
  ['poll_state', 'u16'],
  ['incoming_poll', 'u16'],
  ['', ' ', 2],
  ['FORD_PID_REQUEST_7DF', 'CAN_frame'],
  ['FORD_PID_REQUEST_7E4', 'CAN_frame'],
  ['FORD_ACK_FRAME', 'CAN_frame'],
  ['FORD_DTC_RESET', 'CAN_frame'],
  ['FORD_READ_DTC', 'CAN_frame'],
  ['dtc_buffer', 'u8', 131],
  ['', ' '],
  ['dtc_rx_expected', 'u16'],
  ['dtc_rx_len', 'u16'],
  ['dtc_rx_active', 'b'],
  ['dtc_read_in_progress', 'b'],
  ['', ' ', 2],
  ['dtc_request_millis', 'u32'],
  ['dtc_clear_in_progress', 'b'],
  ['', ' ', 3],
  ['dtc_clear_millis', 'u32'],
  ['FORD_25B', 'CAN_frame'],
  ['FORD_185', 'CAN_frame'],
  ['FORD_167', 'CAN_frame'],
  ['FORD_230', 'CAN_frame'],
  ['FORD_7F', 'CAN_frame'],
  ['FORD_165', 'CAN_frame'],
  ['FORD_7E', 'CAN_frame'],
  ['FORD_4C', 'CAN_frame'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const renderer: [number, string] = [24, 'FordMachEHtmlRenderer'];
export const UserRequestDTCreset: [number, string] = [32, 'b'];
export const UserRequestDTCreadout: [number, string] = [33, 'b'];
export const previousMillis20: [number, string] = [36, 'u32'];
export const previousMillis30: [number, string] = [40, 'u32'];
export const previousMillis50: [number, string] = [44, 'u32'];
export const previousMillis100: [number, string] = [48, 'u32'];
export const previousMillis250: [number, string] = [52, 'u32'];
export const previousMillis1000: [number, string] = [56, 'u32'];
export const previousMillis10s: [number, string] = [60, 'u32'];
export const cell_temperature: [number, string, number] = [64, 'i16', 6];
export const maximum_temperature: [number, string] = [76, 'i16'];
export const minimum_temperature: [number, string] = [78, 'i16'];
export const battery_soc: [number, string] = [80, 'u16'];
export const battery_soh: [number, string] = [82, 'u16'];
export const battery_voltage: [number, string] = [84, 'u16'];
export const battery_current: [number, string] = [86, 'i16'];
export const maximum_cellvoltage_mV: [number, string] = [88, 'u16'];
export const minimum_cellvoltage_mV: [number, string] = [90, 'u16'];
export const charge_current_allowed: [number, string] = [92, 'u16'];
export const discharge_current_allowed: [number, string] = [94, 'u16'];
export const pid_reply: [number, string] = [96, 'u16'];
export const polled_12V: [number, string] = [98, 'u16'];
export const display_soc: [number, string] = [100, 'u8'];
export const currentpoll: [number, string] = [102, 'u16'];
export const poll_index: [number, string] = [104, 'u8'];
export const poll_commands: [number, string] = [106, 'a0e1'];
export const pid_hvb_temp: [number, string] = [178, 'i16'];
export const pid_hvb_soc: [number, string] = [180, 'u32'];
export const pid_hvb_contactor_status: [number, string] = [184, 'u32'];
export const pid_hvb_contactor_positive_leak_voltage: [number, string] = [188, 'u16'];
export const pid_hvb_contactor_negative_leak_voltage: [number, string] = [190, 'u16'];
export const pid_hvb_contactor_positive_voltage: [number, string] = [192, 'u16'];
export const pid_hvb_contactor_negative_voltage: [number, string] = [194, 'u16'];
export const pid_hvb_contactor_positive_bus_leak_resistance: [number, string] = [196, 'u16'];
export const pid_hvb_contactor_negative_bus_leak_resistance: [number, string] = [198, 'u16'];
export const pid_hvb_contactor_overall_leak_resistance: [number, string] = [200, 'u16'];
export const pid_hvb_contactor_open_leak_resistance: [number, string] = [202, 'u16'];
export const pid_hvb_ete: [number, string] = [204, 'u16'];
export const pid_hvb_current: [number, string] = [206, 'u16'];
export const pid_charger_power_limit: [number, string] = [208, 'u16'];
export const pid_hvb_soh: [number, string] = [210, 'u8'];
export const pid_hvb_voltage: [number, string] = [212, 'u16'];
export const pid_hvb_max_charge_current: [number, string] = [214, 'u16'];
export const pid_hvb_charge_voltage_requested: [number, string] = [216, 'u16'];
export const pid_hvb_soc_d: [number, string] = [218, 'u16'];
export const pid_hvb_charge_current_requested: [number, string] = [220, 'u16'];
export const pid_gear_commanded: [number, string] = [222, 'u8'];
export const pid_key_state: [number, string] = [223, 'u8'];
export const pid_charge_plug: [number, string] = [224, 'u8'];
export const pid_charger_output_voltage: [number, string] = [225, 'u8'];
export const pid_charger_status: [number, string] = [226, 'u8'];
export const pid_charger_output_current_measured: [number, string] = [227, 'u8'];
export const pid_evse_type: [number, string] = [228, 'u8'];
export const pid_charger_max_power: [number, string] = [229, 'u8'];
export const pid_charging_status: [number, string] = [230, 'u8'];
export const pid_charger_input_power_available: [number, string] = [231, 'u8'];
export const pid_time: [number, string] = [232, 'u8'];
export const pid_lores_odometer: [number, string] = [233, 'u8'];
export const pid_engine_runtime: [number, string] = [234, 'u8'];
export const pid_hvb_calendar_age_months: [number, string] = [236, 'u16'];
export const pid_battery_capacity_ah: [number, string] = [238, 'u16'];
export const pid_maintenance_rebalance_status: [number, string] = [240, 'u8'];
export const poll_state: [number, string] = [242, 'u16'];
export const incoming_poll: [number, string] = [244, 'u16'];
export const FORD_PID_REQUEST_7DF: [number, string] = [248, 'CAN_frame'];
export const FORD_PID_REQUEST_7E4: [number, string] = [320, 'CAN_frame'];
export const FORD_ACK_FRAME: [number, string] = [392, 'CAN_frame'];
export const FORD_DTC_RESET: [number, string] = [464, 'CAN_frame'];
export const FORD_READ_DTC: [number, string] = [536, 'CAN_frame'];
export const dtc_buffer: [number, string, number] = [608, 'u8', 131];
export const dtc_rx_expected: [number, string] = [740, 'u16'];
export const dtc_rx_len: [number, string] = [742, 'u16'];
export const dtc_rx_active: [number, string] = [744, 'b'];
export const dtc_read_in_progress: [number, string] = [745, 'b'];
export const dtc_request_millis: [number, string] = [748, 'u32'];
export const dtc_clear_in_progress: [number, string] = [752, 'b'];
export const dtc_clear_millis: [number, string] = [756, 'u32'];
export const FORD_25B: [number, string] = [760, 'CAN_frame'];
export const FORD_185: [number, string] = [832, 'CAN_frame'];
export const FORD_167: [number, string] = [904, 'CAN_frame'];
export const FORD_230: [number, string] = [976, 'CAN_frame'];
export const FORD_7F: [number, string] = [1048, 'CAN_frame'];
export const FORD_165: [number, string] = [1120, 'CAN_frame'];
export const FORD_7E: [number, string] = [1192, 'CAN_frame'];
export const FORD_4C: [number, string] = [1264, 'CAN_frame'];
