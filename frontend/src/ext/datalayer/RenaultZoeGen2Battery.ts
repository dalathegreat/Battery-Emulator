// RenaultZoeGen2Battery: 1544 bytes; base classes: CanBattery@0
export const RENAULT_ZOE_GEN2_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['renderer', 'RenaultZoeGen2HtmlRenderer'],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['datalayer_zoePH2', 'DATALAYER_INFO_ZOE_PH2*'],
  ['allows_contactor_closing', 'bool*'],
  ['UserRequestedDTCReset', 'b'],
  ['', ' ', 3],
  ['startTimeNVROL', 'u32'],
  ['balancing_status_cell', 'b', 96],
  ['NVROLstateMachine', 'u8'],
  ['startup_counter', 'u8'],
  ['battery_soc', 'u16'],
  ['battery_usable_soc', 'u16'],
  ['battery_soh', 'u16'],
  ['battery_pack_voltage_polled_dV', 'u16'],
  ['battery_pack_voltage_periodic_dV', 'u16'],
  ['battery_minimum_cell_voltage_mV', 'u16'],
  ['battery_maximum_cell_voltage_mV', 'u16'],
  ['battery_max_cell_voltage_polled', 'u16'],
  ['battery_min_cell_voltage_polled', 'u16'],
  ['battery_12v', 'u16'],
  ['battery_avg_temp', 'u16'],
  ['battery_min_temp', 'u16'],
  ['battery_max_temp', 'u16'],
  ['battery_max_power', 'u16'],
  ['battery_interlock', 'u16'],
  ['battery_interlock_polled', 'u16'],
  ['battery_kwh', 'u16'],
  ['battery_current', 'i32'],
  ['battery_current_offset', 'u16'],
  ['battery_max_generated', 'u16'],
  ['battery_max_available', 'u16'],
  ['battery_current_voltage', 'u16'],
  ['battery_charging_status', 'u16'],
  ['battery_remaining_charge', 'u16'],
  ['battery_balance_capacity_total', 'u16'],
  ['battery_balance_time_total', 'u16'],
  ['battery_balance_capacity_sleep', 'u16'],
  ['battery_balance_time_sleep', 'u16'],
  ['battery_balance_capacity_wake', 'u16'],
  ['battery_balance_time_wake', 'u16'],
  ['battery_bms_state', 'u16'],
  ['battery_balance_switches', 'u16'],
  ['battery_energy_complete', 'u16'],
  ['battery_energy_partial', 'u16'],
  ['battery_slave_failures', 'u32'],
  ['battery_mileage', 'u16'],
  ['battery_fan_speed', 'u16'],
  ['battery_fan_period', 'u16'],
  ['battery_fan_control', 'u16'],
  ['battery_fan_duty', 'u16'],
  ['battery_temporisation', 'u16'],
  ['battery_time', 'u16'],
  ['battery_pack_time', 'u16'],
  ['battery_soc_min', 'u16'],
  ['battery_soc_max', 'u16'],
  ['temporary_variable', 'u16'],
  ['', ' ', 2],
  ['ZOE_376_time_now_s', 'u32'],
  ['kProductionTimestamp_s', 'u32'],
  ['', ' ', 4],
  ['ZOE_0EE', 'CAN_frame'],
  ['ZOE_373', 'CAN_frame'],
  ['ZOE_375', 'CAN_frame'],
  ['ZOE_376', 'CAN_frame'],
  ['ZOE_5F8', 'CAN_frame'],
  ['ZOE_6BF', 'CAN_frame'],
  ['ZOE_POLL_18DADBF1', 'CAN_frame'],
  ['ZOE_POLL_FLOW_CONTROL', 'CAN_frame'],
  ['ZOE_CLEAR_DTC', 'CAN_frame'],
  ['ZOE_NVROL_1_18DADBF1', 'CAN_frame'],
  ['ZOE_NVROL_2_18DADBF1', 'CAN_frame'],
  ['ZOE_SLEEP_1_18DADBF1', 'CAN_frame'],
  ['ZOE_SLEEP_2_18DADBF1', 'CAN_frame'],
  ['poll_commands', 'b62c'],
  ['', ' ', 326],
  ['counter_373', 'u8'],
  ['poll_index', 'u8'],
  ['currentpoll', 'u16'],
  ['reply_poll', 'u16'],
  ['counter_10ms', 'u8'],
  ['', ' ', 3],
  ['previousMillis10', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis200', 'u32'],
  ['previousMillis1000', 'u32'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const renderer: [number, string] = [24, 'RenaultZoeGen2HtmlRenderer'];
export const datalayer_battery: [number, string] = [28, 'DATALAYER_BATTERY_TYPE*'];
export const datalayer_zoePH2: [number, string] = [32, 'DATALAYER_INFO_ZOE_PH2*'];
export const allows_contactor_closing: [number, string] = [36, 'bool*'];
export const UserRequestedDTCReset: [number, string] = [40, 'b'];
export const startTimeNVROL: [number, string] = [44, 'u32'];
export const balancing_status_cell: [number, string, number] = [48, 'b', 96];
export const NVROLstateMachine: [number, string] = [144, 'u8'];
export const startup_counter: [number, string] = [145, 'u8'];
export const battery_soc: [number, string] = [146, 'u16'];
export const battery_usable_soc: [number, string] = [148, 'u16'];
export const battery_soh: [number, string] = [150, 'u16'];
export const battery_pack_voltage_polled_dV: [number, string] = [152, 'u16'];
export const battery_pack_voltage_periodic_dV: [number, string] = [154, 'u16'];
export const battery_minimum_cell_voltage_mV: [number, string] = [156, 'u16'];
export const battery_maximum_cell_voltage_mV: [number, string] = [158, 'u16'];
export const battery_max_cell_voltage_polled: [number, string] = [160, 'u16'];
export const battery_min_cell_voltage_polled: [number, string] = [162, 'u16'];
export const battery_12v: [number, string] = [164, 'u16'];
export const battery_avg_temp: [number, string] = [166, 'u16'];
export const battery_min_temp: [number, string] = [168, 'u16'];
export const battery_max_temp: [number, string] = [170, 'u16'];
export const battery_max_power: [number, string] = [172, 'u16'];
export const battery_interlock: [number, string] = [174, 'u16'];
export const battery_interlock_polled: [number, string] = [176, 'u16'];
export const battery_kwh: [number, string] = [178, 'u16'];
export const battery_current: [number, string] = [180, 'i32'];
export const battery_current_offset: [number, string] = [184, 'u16'];
export const battery_max_generated: [number, string] = [186, 'u16'];
export const battery_max_available: [number, string] = [188, 'u16'];
export const battery_current_voltage: [number, string] = [190, 'u16'];
export const battery_charging_status: [number, string] = [192, 'u16'];
export const battery_remaining_charge: [number, string] = [194, 'u16'];
export const battery_balance_capacity_total: [number, string] = [196, 'u16'];
export const battery_balance_time_total: [number, string] = [198, 'u16'];
export const battery_balance_capacity_sleep: [number, string] = [200, 'u16'];
export const battery_balance_time_sleep: [number, string] = [202, 'u16'];
export const battery_balance_capacity_wake: [number, string] = [204, 'u16'];
export const battery_balance_time_wake: [number, string] = [206, 'u16'];
export const battery_bms_state: [number, string] = [208, 'u16'];
export const battery_balance_switches: [number, string] = [210, 'u16'];
export const battery_energy_complete: [number, string] = [212, 'u16'];
export const battery_energy_partial: [number, string] = [214, 'u16'];
export const battery_slave_failures: [number, string] = [216, 'u32'];
export const battery_mileage: [number, string] = [220, 'u16'];
export const battery_fan_speed: [number, string] = [222, 'u16'];
export const battery_fan_period: [number, string] = [224, 'u16'];
export const battery_fan_control: [number, string] = [226, 'u16'];
export const battery_fan_duty: [number, string] = [228, 'u16'];
export const battery_temporisation: [number, string] = [230, 'u16'];
export const battery_time: [number, string] = [232, 'u16'];
export const battery_pack_time: [number, string] = [234, 'u16'];
export const battery_soc_min: [number, string] = [236, 'u16'];
export const battery_soc_max: [number, string] = [238, 'u16'];
export const temporary_variable: [number, string] = [240, 'u16'];
export const ZOE_376_time_now_s: [number, string] = [244, 'u32'];
export const kProductionTimestamp_s: [number, string] = [248, 'u32'];
export const ZOE_0EE: [number, string] = [256, 'CAN_frame'];
export const ZOE_373: [number, string] = [328, 'CAN_frame'];
export const ZOE_375: [number, string] = [400, 'CAN_frame'];
export const ZOE_376: [number, string] = [472, 'CAN_frame'];
export const ZOE_5F8: [number, string] = [544, 'CAN_frame'];
export const ZOE_6BF: [number, string] = [616, 'CAN_frame'];
export const ZOE_POLL_18DADBF1: [number, string] = [688, 'CAN_frame'];
export const ZOE_POLL_FLOW_CONTROL: [number, string] = [760, 'CAN_frame'];
export const ZOE_CLEAR_DTC: [number, string] = [832, 'CAN_frame'];
export const ZOE_NVROL_1_18DADBF1: [number, string] = [904, 'CAN_frame'];
export const ZOE_NVROL_2_18DADBF1: [number, string] = [976, 'CAN_frame'];
export const ZOE_SLEEP_1_18DADBF1: [number, string] = [1048, 'CAN_frame'];
export const ZOE_SLEEP_2_18DADBF1: [number, string] = [1120, 'CAN_frame'];
export const poll_commands: [number, string] = [1192, 'b62c'];
export const counter_373: [number, string] = [1518, 'u8'];
export const poll_index: [number, string] = [1519, 'u8'];
export const currentpoll: [number, string] = [1520, 'u16'];
export const reply_poll: [number, string] = [1522, 'u16'];
export const counter_10ms: [number, string] = [1524, 'u8'];
export const previousMillis10: [number, string] = [1528, 'u32'];
export const previousMillis100: [number, string] = [1532, 'u32'];
export const previousMillis200: [number, string] = [1536, 'u32'];
export const previousMillis1000: [number, string] = [1540, 'u32'];
