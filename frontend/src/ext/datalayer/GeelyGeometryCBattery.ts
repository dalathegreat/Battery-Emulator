// GeelyGeometryCBattery: 2128 bytes; base classes: CanBattery@0
export const GEELY_GEOMETRY_CBATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['renderer', 'GeelyGeometryCHtmlRenderer'],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['datalayer_geometryc', 'DATALAYER_INFO_GEELY_GEOMETRY_C*'],
  ['UserRequestDTCreset', 'b'],
  ['', ' ', 3],
  ['GEELY_191', 'CAN_frame'],
  ['GEELY_2D2', 'CAN_frame'],
  ['GEELY_0A6', 'CAN_frame'],
  ['GEELY_160', 'CAN_frame'],
  ['GEELY_165', 'CAN_frame'],
  ['GEELY_1A4', 'CAN_frame'],
  ['GEELY_162', 'CAN_frame'],
  ['GEELY_1A5', 'CAN_frame'],
  ['GEELY_1B2', 'CAN_frame'],
  ['GEELY_221', 'CAN_frame'],
  ['GEELY_220', 'CAN_frame'],
  ['GEELY_1A3', 'CAN_frame'],
  ['GEELY_1A7', 'CAN_frame'],
  ['GEELY_0A8', 'CAN_frame'],
  ['GEELY_1F2', 'CAN_frame'],
  ['GEELY_222', 'CAN_frame'],
  ['GEELY_1A6', 'CAN_frame'],
  ['GEELY_145', 'CAN_frame'],
  ['GEELY_0E0', 'CAN_frame'],
  ['GEELY_0F9', 'CAN_frame'],
  ['GEELY_292', 'CAN_frame'],
  ['GEELY_0FA', 'CAN_frame'],
  ['GEELY_197', 'CAN_frame'],
  ['GEELY_150', 'CAN_frame'],
  ['GEELY_POLL', 'CAN_frame'],
  ['GEELY_ACK', 'CAN_frame'],
  ['GEELY_CLEAR_DTC', 'CAN_frame'],
  ['poll_pid', 'u16'],
  ['incoming_poll', 'u16'],
  ['counter_10ms', 'u8'],
  ['counter_20ms', 'u8'],
  ['counter_50ms', 'u8'],
  ['counter_100ms', 'u8'],
  ['previousMillis10', 'u32'],
  ['previousMillis20', 'u32'],
  ['previousMillis50', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis200', 'u32'],
  ['mux', 'u8'],
  ['', ' '],
  ['battery_voltage', 'u16'],
  ['maximum_temperature', 'i16'],
  ['minimum_temperature', 'i16'],
  ['HVIL_signal', 'u8'],
  ['serialnumbers', 'u8', 28],
  ['', ' '],
  ['maximum_cell_voltage', 'u16'],
  ['discharge_power_allowed', 'u16'],
  ['poll_soc', 'u16'],
  ['poll_cc2_voltage', 'u16'],
  ['poll_cell_max_voltage_number', 'u16'],
  ['poll_cell_min_voltage_number', 'u16'],
  ['poll_amount_cells', 'u16'],
  ['poll_specificial_voltage', 'u16'],
  ['poll_unknown1', 'u16'],
  ['poll_raw_soc_max', 'u16'],
  ['poll_raw_soc_min', 'u16'],
  ['poll_unknown4', 'u16'],
  ['poll_cap_module_max', 'u16'],
  ['poll_cap_module_min', 'u16'],
  ['poll_unknown7', 'u16'],
  ['poll_unknown8', 'u16'],
  ['poll_temperature', 'i16', 6],
  ['poll_software_version', 'u8', 16],
  ['poll_hardware_version', 'u8', 16],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const renderer: [number, string] = [24, 'GeelyGeometryCHtmlRenderer'];
export const datalayer_battery: [number, string] = [28, 'DATALAYER_BATTERY_TYPE*'];
export const datalayer_geometryc: [number, string] = [32, 'DATALAYER_INFO_GEELY_GEOMETRY_C*'];
export const UserRequestDTCreset: [number, string] = [36, 'b'];
export const GEELY_191: [number, string] = [40, 'CAN_frame'];
export const GEELY_2D2: [number, string] = [112, 'CAN_frame'];
export const GEELY_0A6: [number, string] = [184, 'CAN_frame'];
export const GEELY_160: [number, string] = [256, 'CAN_frame'];
export const GEELY_165: [number, string] = [328, 'CAN_frame'];
export const GEELY_1A4: [number, string] = [400, 'CAN_frame'];
export const GEELY_162: [number, string] = [472, 'CAN_frame'];
export const GEELY_1A5: [number, string] = [544, 'CAN_frame'];
export const GEELY_1B2: [number, string] = [616, 'CAN_frame'];
export const GEELY_221: [number, string] = [688, 'CAN_frame'];
export const GEELY_220: [number, string] = [760, 'CAN_frame'];
export const GEELY_1A3: [number, string] = [832, 'CAN_frame'];
export const GEELY_1A7: [number, string] = [904, 'CAN_frame'];
export const GEELY_0A8: [number, string] = [976, 'CAN_frame'];
export const GEELY_1F2: [number, string] = [1048, 'CAN_frame'];
export const GEELY_222: [number, string] = [1120, 'CAN_frame'];
export const GEELY_1A6: [number, string] = [1192, 'CAN_frame'];
export const GEELY_145: [number, string] = [1264, 'CAN_frame'];
export const GEELY_0E0: [number, string] = [1336, 'CAN_frame'];
export const GEELY_0F9: [number, string] = [1408, 'CAN_frame'];
export const GEELY_292: [number, string] = [1480, 'CAN_frame'];
export const GEELY_0FA: [number, string] = [1552, 'CAN_frame'];
export const GEELY_197: [number, string] = [1624, 'CAN_frame'];
export const GEELY_150: [number, string] = [1696, 'CAN_frame'];
export const GEELY_POLL: [number, string] = [1768, 'CAN_frame'];
export const GEELY_ACK: [number, string] = [1840, 'CAN_frame'];
export const GEELY_CLEAR_DTC: [number, string] = [1912, 'CAN_frame'];
export const poll_pid: [number, string] = [1984, 'u16'];
export const incoming_poll: [number, string] = [1986, 'u16'];
export const counter_10ms: [number, string] = [1988, 'u8'];
export const counter_20ms: [number, string] = [1989, 'u8'];
export const counter_50ms: [number, string] = [1990, 'u8'];
export const counter_100ms: [number, string] = [1991, 'u8'];
export const previousMillis10: [number, string] = [1992, 'u32'];
export const previousMillis20: [number, string] = [1996, 'u32'];
export const previousMillis50: [number, string] = [2000, 'u32'];
export const previousMillis100: [number, string] = [2004, 'u32'];
export const previousMillis200: [number, string] = [2008, 'u32'];
export const mux: [number, string] = [2012, 'u8'];
export const battery_voltage: [number, string] = [2014, 'u16'];
export const maximum_temperature: [number, string] = [2016, 'i16'];
export const minimum_temperature: [number, string] = [2018, 'i16'];
export const HVIL_signal: [number, string] = [2020, 'u8'];
export const serialnumbers: [number, string, number] = [2021, 'u8', 28];
export const maximum_cell_voltage: [number, string] = [2050, 'u16'];
export const discharge_power_allowed: [number, string] = [2052, 'u16'];
export const poll_soc: [number, string] = [2054, 'u16'];
export const poll_cc2_voltage: [number, string] = [2056, 'u16'];
export const poll_cell_max_voltage_number: [number, string] = [2058, 'u16'];
export const poll_cell_min_voltage_number: [number, string] = [2060, 'u16'];
export const poll_amount_cells: [number, string] = [2062, 'u16'];
export const poll_specificial_voltage: [number, string] = [2064, 'u16'];
export const poll_unknown1: [number, string] = [2066, 'u16'];
export const poll_raw_soc_max: [number, string] = [2068, 'u16'];
export const poll_raw_soc_min: [number, string] = [2070, 'u16'];
export const poll_unknown4: [number, string] = [2072, 'u16'];
export const poll_cap_module_max: [number, string] = [2074, 'u16'];
export const poll_cap_module_min: [number, string] = [2076, 'u16'];
export const poll_unknown7: [number, string] = [2078, 'u16'];
export const poll_unknown8: [number, string] = [2080, 'u16'];
export const poll_temperature: [number, string, number] = [2082, 'i16', 6];
export const poll_software_version: [number, string, number] = [2094, 'u8', 16];
export const poll_hardware_version: [number, string, number] = [2110, 'u8', 16];
