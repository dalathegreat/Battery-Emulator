// FoxessBattery: 216 bytes; base classes: CanBattery@0
export const FOXESS_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['previousMillis500', 'u32'],
  ['', ' ', 4],
  ['FOX_1871', 'CAN_frame'],
  ['total_watt_hours', 'u32'],
  ['max_charge_power_dA', 'u16'],
  ['max_discharge_power_dA', 'u16'],
  ['cut_mv_max', 'u16'],
  ['cut_mv_min', 'u16'],
  ['cycle_count', 'u16'],
  ['max_ac_voltage', 'u16'],
  ['temperature_average', 'i16'],
  ['pack1_current_sensor', 'i16'],
  ['pack2_current_sensor', 'i16'],
  ['pack3_current_sensor', 'i16'],
  ['pack4_current_sensor', 'i16'],
  ['pack5_current_sensor', 'i16'],
  ['pack6_current_sensor', 'i16'],
  ['pack7_current_sensor', 'i16'],
  ['pack8_current_sensor', 'i16'],
  ['pack1_temperature_avg_high', 'i16'],
  ['pack2_temperature_avg_high', 'i16'],
  ['pack3_temperature_avg_high', 'i16'],
  ['pack4_temperature_avg_high', 'i16'],
  ['pack5_temperature_avg_high', 'i16'],
  ['pack6_temperature_avg_high', 'i16'],
  ['pack7_temperature_avg_high', 'i16'],
  ['pack8_temperature_avg_high', 'i16'],
  ['pack1_temperature_avg_low', 'i16'],
  ['pack2_temperature_avg_low', 'i16'],
  ['pack3_temperature_avg_low', 'i16'],
  ['pack4_temperature_avg_low', 'i16'],
  ['pack5_temperature_avg_low', 'i16'],
  ['pack6_temperature_avg_low', 'i16'],
  ['pack7_temperature_avg_low', 'i16'],
  ['pack8_temperature_avg_low', 'i16'],
  ['pack1_voltage', 'u16'],
  ['pack2_voltage', 'u16'],
  ['pack3_voltage', 'u16'],
  ['pack4_voltage', 'u16'],
  ['pack5_voltage', 'u16'],
  ['pack6_voltage', 'u16'],
  ['pack7_voltage', 'u16'],
  ['pack8_voltage', 'u16'],
  ['pack1_SOC', 'u8'],
  ['pack2_SOC', 'u8'],
  ['pack3_SOC', 'u8'],
  ['pack4_SOC', 'u8'],
  ['pack5_SOC', 'u8'],
  ['pack6_SOC', 'u8'],
  ['pack7_SOC', 'u8'],
  ['pack8_SOC', 'u8'],
  ['pack_error', 'u8'],
  ['firmware_pack_minor', 'u8'],
  ['firmware_pack_major', 'u8'],
  ['STATUS_OPERATIONAL_PACKS', 'u8'],
  ['NUMBER_OF_PACKS', 'u8'],
  ['bms_limits_received', 'b'],
  ['contactor_status', 'u8'],
  ['statemachine_polling', 'u8'],
  ['charging_disabled', 'b'],
  ['b0_idle', 'b'],
  ['b1_ok_discharge', 'b'],
  ['b2_ok_charge', 'b'],
  ['b3_discharging', 'b'],
  ['b4_charging', 'b'],
  ['b5_operational', 'b'],
  ['b6_active_error', 'b'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const previousMillis500: [number, string] = [24, 'u32'];
export const FOX_1871: [number, string] = [32, 'CAN_frame'];
export const total_watt_hours: [number, string] = [104, 'u32'];
export const max_charge_power_dA: [number, string] = [108, 'u16'];
export const max_discharge_power_dA: [number, string] = [110, 'u16'];
export const cut_mv_max: [number, string] = [112, 'u16'];
export const cut_mv_min: [number, string] = [114, 'u16'];
export const cycle_count: [number, string] = [116, 'u16'];
export const max_ac_voltage: [number, string] = [118, 'u16'];
export const temperature_average: [number, string] = [120, 'i16'];
export const pack1_current_sensor: [number, string] = [122, 'i16'];
export const pack2_current_sensor: [number, string] = [124, 'i16'];
export const pack3_current_sensor: [number, string] = [126, 'i16'];
export const pack4_current_sensor: [number, string] = [128, 'i16'];
export const pack5_current_sensor: [number, string] = [130, 'i16'];
export const pack6_current_sensor: [number, string] = [132, 'i16'];
export const pack7_current_sensor: [number, string] = [134, 'i16'];
export const pack8_current_sensor: [number, string] = [136, 'i16'];
export const pack1_temperature_avg_high: [number, string] = [138, 'i16'];
export const pack2_temperature_avg_high: [number, string] = [140, 'i16'];
export const pack3_temperature_avg_high: [number, string] = [142, 'i16'];
export const pack4_temperature_avg_high: [number, string] = [144, 'i16'];
export const pack5_temperature_avg_high: [number, string] = [146, 'i16'];
export const pack6_temperature_avg_high: [number, string] = [148, 'i16'];
export const pack7_temperature_avg_high: [number, string] = [150, 'i16'];
export const pack8_temperature_avg_high: [number, string] = [152, 'i16'];
export const pack1_temperature_avg_low: [number, string] = [154, 'i16'];
export const pack2_temperature_avg_low: [number, string] = [156, 'i16'];
export const pack3_temperature_avg_low: [number, string] = [158, 'i16'];
export const pack4_temperature_avg_low: [number, string] = [160, 'i16'];
export const pack5_temperature_avg_low: [number, string] = [162, 'i16'];
export const pack6_temperature_avg_low: [number, string] = [164, 'i16'];
export const pack7_temperature_avg_low: [number, string] = [166, 'i16'];
export const pack8_temperature_avg_low: [number, string] = [168, 'i16'];
export const pack1_voltage: [number, string] = [170, 'u16'];
export const pack2_voltage: [number, string] = [172, 'u16'];
export const pack3_voltage: [number, string] = [174, 'u16'];
export const pack4_voltage: [number, string] = [176, 'u16'];
export const pack5_voltage: [number, string] = [178, 'u16'];
export const pack6_voltage: [number, string] = [180, 'u16'];
export const pack7_voltage: [number, string] = [182, 'u16'];
export const pack8_voltage: [number, string] = [184, 'u16'];
export const pack1_SOC: [number, string] = [186, 'u8'];
export const pack2_SOC: [number, string] = [187, 'u8'];
export const pack3_SOC: [number, string] = [188, 'u8'];
export const pack4_SOC: [number, string] = [189, 'u8'];
export const pack5_SOC: [number, string] = [190, 'u8'];
export const pack6_SOC: [number, string] = [191, 'u8'];
export const pack7_SOC: [number, string] = [192, 'u8'];
export const pack8_SOC: [number, string] = [193, 'u8'];
export const pack_error: [number, string] = [194, 'u8'];
export const firmware_pack_minor: [number, string] = [195, 'u8'];
export const firmware_pack_major: [number, string] = [196, 'u8'];
export const STATUS_OPERATIONAL_PACKS: [number, string] = [197, 'u8'];
export const NUMBER_OF_PACKS: [number, string] = [198, 'u8'];
export const bms_limits_received: [number, string] = [199, 'b'];
export const contactor_status: [number, string] = [200, 'u8'];
export const statemachine_polling: [number, string] = [201, 'u8'];
export const charging_disabled: [number, string] = [202, 'b'];
export const b0_idle: [number, string] = [203, 'b'];
export const b1_ok_discharge: [number, string] = [204, 'b'];
export const b2_ok_charge: [number, string] = [205, 'b'];
export const b3_discharging: [number, string] = [206, 'b'];
export const b4_charging: [number, string] = [207, 'b'];
export const b5_operational: [number, string] = [208, 'b'];
export const b6_active_error: [number, string] = [209, 'b'];
