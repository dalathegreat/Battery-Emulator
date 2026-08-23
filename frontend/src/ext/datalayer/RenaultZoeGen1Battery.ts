// RenaultZoeGen1Battery: 624 bytes; base classes: CanBattery@0
export const RENAULT_ZOE_GEN1_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['renderer', 'RenaultZoeGen1HtmlRenderer'],
  ['UserRequestedDTCReset', 'b'],
  ['', ' ', 3],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['datalayer_zoe', 'DATALAYER_INFO_ZOE*'],
  ['allows_contactor_closing', 'bool*'],
  ['previousMillis100', 'u32'],
  ['previousMillis250', 'u32'],
  ['counter_423', 'u8'],
  ['', ' ', 3],
  ['ZOE_423', 'CAN_frame'],
  ['ZOE_POLL_79B', 'CAN_frame'],
  ['ZOE_ACK_79B', 'CAN_frame'],
  ['ZOE_CLEAR_DTC', 'CAN_frame'],
  ['LB_SOC', 'u16'],
  ['LB_Display_SOC', 'u16'],
  ['LB_SOH', 'u16'],
  ['LB_Average_Temperature', 'i16'],
  ['LB_Charging_Power_W', 'u32'],
  ['LB_Regen_allowed_W', 'u32'],
  ['LB_Discharge_allowed_W', 'u32'],
  ['LB_Current_raw', 'i16'],
  ['LB_Cell_minimum_temperature', 'i16'],
  ['LB_Cell_maximum_temperature', 'i16'],
  ['LB_Cell_minimum_voltage', 'u16'],
  ['LB_Cell_maximum_voltage', 'u16'],
  ['LB_kWh_Remaining', 'u16'],
  ['LB_Battery_Voltage', 'u16'],
  ['LB_Heartbeat', 'u8'],
  ['LB_CUV', 'u8'],
  ['LB_HVBIR', 'u8'],
  ['LB_HVBUV', 'u8'],
  ['LB_EOCR', 'u8'],
  ['LB_HVBOC', 'u8'],
  ['LB_HVBOT', 'u8'],
  ['LB_HVBOV', 'u8'],
  ['LB_COV', 'u8'],
  ['frame0', 'u8'],
  ['current_poll', 'u8'],
  ['requested_poll', 'u8'],
  ['group', 'u8'],
  ['', ' '],
  ['cellvoltages', 'u16', 96],
  ['calculated_total_pack_voltage_mV', 'u32'],
  ['highbyte_cell_next_frame', 'u8'],
  ['', ' '],
  ['SOC_polled', 'u16'],
  ['cell_1_temperature_polled', 'i16'],
  ['cell_2_temperature_polled', 'i16'],
  ['cell_3_temperature_polled', 'i16'],
  ['cell_4_temperature_polled', 'i16'],
  ['cell_5_temperature_polled', 'i16'],
  ['cell_6_temperature_polled', 'i16'],
  ['cell_7_temperature_polled', 'i16'],
  ['cell_8_temperature_polled', 'i16'],
  ['cell_9_temperature_polled', 'i16'],
  ['cell_10_temperature_polled', 'i16'],
  ['cell_11_temperature_polled', 'i16'],
  ['cell_12_temperature_polled', 'i16'],
  ['battery_mileage_in_km', 'u16'],
  ['kWh_from_beginning_of_battery_life', 'u16'],
  ['looping_over_20', 'b'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const renderer: [number, string] = [24, 'RenaultZoeGen1HtmlRenderer'];
export const UserRequestedDTCReset: [number, string] = [28, 'b'];
export const datalayer_battery: [number, string] = [32, 'DATALAYER_BATTERY_TYPE*'];
export const datalayer_zoe: [number, string] = [36, 'DATALAYER_INFO_ZOE*'];
export const allows_contactor_closing: [number, string] = [40, 'bool*'];
export const previousMillis100: [number, string] = [44, 'u32'];
export const previousMillis250: [number, string] = [48, 'u32'];
export const counter_423: [number, string] = [52, 'u8'];
export const ZOE_423: [number, string] = [56, 'CAN_frame'];
export const ZOE_POLL_79B: [number, string] = [128, 'CAN_frame'];
export const ZOE_ACK_79B: [number, string] = [200, 'CAN_frame'];
export const ZOE_CLEAR_DTC: [number, string] = [272, 'CAN_frame'];
export const LB_SOC: [number, string] = [344, 'u16'];
export const LB_Display_SOC: [number, string] = [346, 'u16'];
export const LB_SOH: [number, string] = [348, 'u16'];
export const LB_Average_Temperature: [number, string] = [350, 'i16'];
export const LB_Charging_Power_W: [number, string] = [352, 'u32'];
export const LB_Regen_allowed_W: [number, string] = [356, 'u32'];
export const LB_Discharge_allowed_W: [number, string] = [360, 'u32'];
export const LB_Current_raw: [number, string] = [364, 'i16'];
export const LB_Cell_minimum_temperature: [number, string] = [366, 'i16'];
export const LB_Cell_maximum_temperature: [number, string] = [368, 'i16'];
export const LB_Cell_minimum_voltage: [number, string] = [370, 'u16'];
export const LB_Cell_maximum_voltage: [number, string] = [372, 'u16'];
export const LB_kWh_Remaining: [number, string] = [374, 'u16'];
export const LB_Battery_Voltage: [number, string] = [376, 'u16'];
export const LB_Heartbeat: [number, string] = [378, 'u8'];
export const LB_CUV: [number, string] = [379, 'u8'];
export const LB_HVBIR: [number, string] = [380, 'u8'];
export const LB_HVBUV: [number, string] = [381, 'u8'];
export const LB_EOCR: [number, string] = [382, 'u8'];
export const LB_HVBOC: [number, string] = [383, 'u8'];
export const LB_HVBOT: [number, string] = [384, 'u8'];
export const LB_HVBOV: [number, string] = [385, 'u8'];
export const LB_COV: [number, string] = [386, 'u8'];
export const frame0: [number, string] = [387, 'u8'];
export const current_poll: [number, string] = [388, 'u8'];
export const requested_poll: [number, string] = [389, 'u8'];
export const group: [number, string] = [390, 'u8'];
export const cellvoltages: [number, string, number] = [392, 'u16', 96];
export const calculated_total_pack_voltage_mV: [number, string] = [584, 'u32'];
export const highbyte_cell_next_frame: [number, string] = [588, 'u8'];
export const SOC_polled: [number, string] = [590, 'u16'];
export const cell_1_temperature_polled: [number, string] = [592, 'i16'];
export const cell_2_temperature_polled: [number, string] = [594, 'i16'];
export const cell_3_temperature_polled: [number, string] = [596, 'i16'];
export const cell_4_temperature_polled: [number, string] = [598, 'i16'];
export const cell_5_temperature_polled: [number, string] = [600, 'i16'];
export const cell_6_temperature_polled: [number, string] = [602, 'i16'];
export const cell_7_temperature_polled: [number, string] = [604, 'i16'];
export const cell_8_temperature_polled: [number, string] = [606, 'i16'];
export const cell_9_temperature_polled: [number, string] = [608, 'i16'];
export const cell_10_temperature_polled: [number, string] = [610, 'i16'];
export const cell_11_temperature_polled: [number, string] = [612, 'i16'];
export const cell_12_temperature_polled: [number, string] = [614, 'i16'];
export const battery_mileage_in_km: [number, string] = [616, 'u16'];
export const kWh_from_beginning_of_battery_life: [number, string] = [618, 'u16'];
export const looping_over_20: [number, string] = [620, 'b'];
