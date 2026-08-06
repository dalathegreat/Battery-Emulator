// BoltAmperaBattery: 1248 bytes; base classes: CanBattery@0
export const BOLT_AMPERA_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['renderer', 'BoltAmperaHtmlRenderer'],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['datalayer_boltampera', 'DATALAYER_INFO_BOLTAMPERA*'],
  ['allows_contactor_closing', 'bool*'],
  ['UserRequestDTCreset', 'b'],
  ['', ' ', 3],
  ['previousMillis20ms', 'u32'],
  ['previousMillis100ms', 'u32'],
  ['previousMillis120ms', 'u32'],
  ['', ' ', 4],
  ['BOLT_778', 'CAN_frame'],
  ['BOLT_POLL_7E4', 'CAN_frame'],
  ['BOLT_ACK_7E4', 'CAN_frame'],
  ['BOLT_POLL_7E7', 'CAN_frame'],
  ['BOLT_ACK_7E7', 'CAN_frame'],
  ['BOLT_CLEAR_DTC', 'CAN_frame'],
  ['soc_periodic', 'u16'],
  ['battery_cell_voltages', 'u16', 96],
  ['cellblock_voltage', 'u16', 96],
  ['', ' ', 2],
  ['sensed_battery_voltage_mV', 'u32'],
  ['sensed_current_sensor_1', 'i16'],
  ['sensed_current_sensor_2', 'i16'],
  ['battery_capacity_my17_18', 'u16'],
  ['battery_capacity_my19plus', 'u16'],
  ['battery_SOC_display', 'u16'],
  ['battery_SOC_raw_highprec', 'u16'],
  ['battery_max_temperature', 'u16'],
  ['battery_min_temperature', 'u16'],
  ['battery_min_cell_voltage', 'u16'],
  ['battery_max_cell_voltage', 'u16'],
  ['battery_internal_resistance', 'u16'],
  ['battery_lowest_cell', 'u16'],
  ['battery_highest_cell', 'u16'],
  ['battery_voltage_polled', 'u16'],
  ['battery_voltage_periodic_dV', 'u16'],
  ['battery_vehicle_isolation', 'u16'],
  ['battery_isolation_kohm', 'u16'],
  ['battery_HV_locked', 'u16'],
  ['battery_crash_event', 'u16'],
  ['battery_HVIL', 'u16'],
  ['battery_HVIL_status', 'u16'],
  ['battery_5V_ref', 'u16'],
  ['battery_current_7E4', 'i16'],
  ['battery_module_temp_1', 'i16'],
  ['battery_module_temp_2', 'i16'],
  ['battery_module_temp_3', 'i16'],
  ['battery_module_temp_4', 'i16'],
  ['battery_module_temp_5', 'i16'],
  ['battery_module_temp_6', 'i16'],
  ['battery_cell_voltage_max_mV', 'u16'],
  ['battery_cell_voltage_min_mV', 'u16'],
  ['battery_cell_average_voltage', 'u16'],
  ['battery_cell_average_voltage_2', 'u16'],
  ['battery_terminal_voltage', 'u16'],
  ['battery_ignition_power_mode', 'u16'],
  ['battery_current_7E7', 'i16'],
  ['inlet_coolant_temperature', 'i16'],
  ['outlet_coolant_temperature', 'i16'],
  ['temperature_1', 'i16'],
  ['temperature_2', 'i16'],
  ['temperature_3', 'i16'],
  ['temperature_4', 'i16'],
  ['temperature_5', 'i16'],
  ['temperature_6', 'i16'],
  ['temperature_highest_C', 'i16'],
  ['temperature_lowest_C', 'i16'],
  ['poll_index_7E4', 'u8'],
  ['', ' '],
  ['currentpoll_7E4', 'u16'],
  ['reply_poll_7E4', 'u16'],
  ['poll_index_7E7', 'u8'],
  ['', ' '],
  ['currentpoll_7E7', 'u16'],
  ['reply_poll_7E7', 'u16'],
  ['poll_commands_7E4', '8d0e'],
  ['', ' ', 38],
  ['poll_commands_7E7', '8d23'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const renderer: [number, string] = [24, 'BoltAmperaHtmlRenderer'];
export const datalayer_battery: [number, string] = [32, 'DATALAYER_BATTERY_TYPE*'];
export const datalayer_boltampera: [number, string] = [36, 'DATALAYER_INFO_BOLTAMPERA*'];
export const allows_contactor_closing: [number, string] = [40, 'bool*'];
export const UserRequestDTCreset: [number, string] = [44, 'b'];
export const previousMillis20ms: [number, string] = [48, 'u32'];
export const previousMillis100ms: [number, string] = [52, 'u32'];
export const previousMillis120ms: [number, string] = [56, 'u32'];
export const BOLT_778: [number, string] = [64, 'CAN_frame'];
export const BOLT_POLL_7E4: [number, string] = [136, 'CAN_frame'];
export const BOLT_ACK_7E4: [number, string] = [208, 'CAN_frame'];
export const BOLT_POLL_7E7: [number, string] = [280, 'CAN_frame'];
export const BOLT_ACK_7E7: [number, string] = [352, 'CAN_frame'];
export const BOLT_CLEAR_DTC: [number, string] = [424, 'CAN_frame'];
export const soc_periodic: [number, string] = [496, 'u16'];
export const battery_cell_voltages: [number, string, number] = [498, 'u16', 96];
export const cellblock_voltage: [number, string, number] = [690, 'u16', 96];
export const sensed_battery_voltage_mV: [number, string] = [884, 'u32'];
export const sensed_current_sensor_1: [number, string] = [888, 'i16'];
export const sensed_current_sensor_2: [number, string] = [890, 'i16'];
export const battery_capacity_my17_18: [number, string] = [892, 'u16'];
export const battery_capacity_my19plus: [number, string] = [894, 'u16'];
export const battery_SOC_display: [number, string] = [896, 'u16'];
export const battery_SOC_raw_highprec: [number, string] = [898, 'u16'];
export const battery_max_temperature: [number, string] = [900, 'u16'];
export const battery_min_temperature: [number, string] = [902, 'u16'];
export const battery_min_cell_voltage: [number, string] = [904, 'u16'];
export const battery_max_cell_voltage: [number, string] = [906, 'u16'];
export const battery_internal_resistance: [number, string] = [908, 'u16'];
export const battery_lowest_cell: [number, string] = [910, 'u16'];
export const battery_highest_cell: [number, string] = [912, 'u16'];
export const battery_voltage_polled: [number, string] = [914, 'u16'];
export const battery_voltage_periodic_dV: [number, string] = [916, 'u16'];
export const battery_vehicle_isolation: [number, string] = [918, 'u16'];
export const battery_isolation_kohm: [number, string] = [920, 'u16'];
export const battery_HV_locked: [number, string] = [922, 'u16'];
export const battery_crash_event: [number, string] = [924, 'u16'];
export const battery_HVIL: [number, string] = [926, 'u16'];
export const battery_HVIL_status: [number, string] = [928, 'u16'];
export const battery_5V_ref: [number, string] = [930, 'u16'];
export const battery_current_7E4: [number, string] = [932, 'i16'];
export const battery_module_temp_1: [number, string] = [934, 'i16'];
export const battery_module_temp_2: [number, string] = [936, 'i16'];
export const battery_module_temp_3: [number, string] = [938, 'i16'];
export const battery_module_temp_4: [number, string] = [940, 'i16'];
export const battery_module_temp_5: [number, string] = [942, 'i16'];
export const battery_module_temp_6: [number, string] = [944, 'i16'];
export const battery_cell_voltage_max_mV: [number, string] = [946, 'u16'];
export const battery_cell_voltage_min_mV: [number, string] = [948, 'u16'];
export const battery_cell_average_voltage: [number, string] = [950, 'u16'];
export const battery_cell_average_voltage_2: [number, string] = [952, 'u16'];
export const battery_terminal_voltage: [number, string] = [954, 'u16'];
export const battery_ignition_power_mode: [number, string] = [956, 'u16'];
export const battery_current_7E7: [number, string] = [958, 'i16'];
export const inlet_coolant_temperature: [number, string] = [960, 'i16'];
export const outlet_coolant_temperature: [number, string] = [962, 'i16'];
export const temperature_1: [number, string] = [964, 'i16'];
export const temperature_2: [number, string] = [966, 'i16'];
export const temperature_3: [number, string] = [968, 'i16'];
export const temperature_4: [number, string] = [970, 'i16'];
export const temperature_5: [number, string] = [972, 'i16'];
export const temperature_6: [number, string] = [974, 'i16'];
export const temperature_highest_C: [number, string] = [976, 'i16'];
export const temperature_lowest_C: [number, string] = [978, 'i16'];
export const poll_index_7E4: [number, string] = [980, 'u8'];
export const currentpoll_7E4: [number, string] = [982, 'u16'];
export const reply_poll_7E4: [number, string] = [984, 'u16'];
export const poll_index_7E7: [number, string] = [986, 'u8'];
export const currentpoll_7E7: [number, string] = [988, 'u16'];
export const reply_poll_7E7: [number, string] = [990, 'u16'];
export const poll_commands_7E4: [number, string] = [992, '8d0e'];
export const poll_commands_7E7: [number, string] = [1030, '8d23'];
