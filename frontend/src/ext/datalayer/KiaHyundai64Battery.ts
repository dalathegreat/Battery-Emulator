// KiaHyundai64Battery: 1096 bytes; base classes: CanBattery@0
export const KIA_HYUNDAI64_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['renderer', 'KiaHyundai64HtmlRenderer'],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['datalayer_battery_extended', 'DATALAYER_INFO_KIAHYUNDAI64*'],
  ['UserRequestDTCreset', 'b'],
  ['', ' ', 3],
  ['allows_contactor_closing', 'bool*'],
  ['contactor_closing_allowed', 'bool*'],
  ['previousMillis100', 'u32'],
  ['previousMillis10', 'u32'],
  ['soc_calculated', 'u16'],
  ['SOC_BMS', 'u16'],
  ['SOC_Display', 'u16'],
  ['batterySOH', 'u16'],
  ['CellVoltMax_mV', 'u16'],
  ['CellVoltMin_mV', 'u16'],
  ['allowedDischargePower', 'u16'],
  ['allowedChargePower', 'u16'],
  ['batteryVoltage', 'u16'],
  ['inverterVoltageFrameHigh', 'u16'],
  ['inverterVoltage', 'u16'],
  ['cellvoltages_mv', 'u16', 98],
  ['leadAcidBatteryVoltage', 'u16'],
  ['batteryAmps', 'i16'],
  ['temperatureMax', 'i16'],
  ['temperatureMin', 'i16'],
  ['poll_data_pid', 'u8'],
  ['open_state', 'u8'],
  ['pid_reply', 'u16'],
  ['holdPidCounter', 'b'],
  ['CellVmaxNo', 'u8'],
  ['CellVminNo', 'u8'],
  ['batteryManagementMode', 'u8'],
  ['BMS_ign', 'u8'],
  ['batteryRelay', 'u8'],
  ['waterleakageSensor', 'u8'],
  ['counter_200', 'u8'],
  ['temperature_water_inlet', 'i8'],
  ['heatertemp', 'i8'],
  ['powerRelayTemperature', 'i8'],
  ['startedUp', 'b'],
  ['ecu_serial_number', 'u8', 16],
  ['ecu_version_number', 'u8', 16],
  ['', ' ', 2],
  ['cumulative_charge_current_ah', 'u32'],
  ['cumulative_discharge_current_ah', 'u32'],
  ['cumulative_energy_charged_kWh', 'u32'],
  ['cumulative_energy_discharged_HIGH_BYTE', 'u16'],
  ['', ' ', 2],
  ['cumulative_energy_discharged_kWh', 'u32'],
  ['powered_on_total_time', 'u32'],
  ['isolation_resistance_kOhm', 'u16'],
  ['number_of_standard_charging_sessions', 'u16'],
  ['number_of_fastcharging_sessions', 'u16'],
  ['accumulated_normal_charging_energy_kWh', 'u16'],
  ['accumulated_fastcharging_energy_kWh', 'u16'],
  ['', ' ', 6],
  ['KIA_HYUNDAI_200', 'CAN_frame'],
  ['KIA_HYUNDAI_523', 'CAN_frame'],
  ['KIA_HYUNDAI_524', 'CAN_frame'],
  ['KIA64_553', 'CAN_frame'],
  ['KIA64_57F', 'CAN_frame'],
  ['KIA64_2A1', 'CAN_frame'],
  ['KIA64_7E4_OPEN_CONTACTOR_SEQUENCE', 'CAN_frame'],
  ['KIA64_7E4_poll', 'CAN_frame'],
  ['KIA64_7E4_ack', 'CAN_frame'],
  ['KIA64_CLEAR_DTC', 'CAN_frame'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const renderer: [number, string] = [24, 'KiaHyundai64HtmlRenderer'];
export const datalayer_battery: [number, string] = [32, 'DATALAYER_BATTERY_TYPE*'];
export const datalayer_battery_extended: [number, string] = [36, 'DATALAYER_INFO_KIAHYUNDAI64*'];
export const UserRequestDTCreset: [number, string] = [40, 'b'];
export const allows_contactor_closing: [number, string] = [44, 'bool*'];
export const contactor_closing_allowed: [number, string] = [48, 'bool*'];
export const previousMillis100: [number, string] = [52, 'u32'];
export const previousMillis10: [number, string] = [56, 'u32'];
export const soc_calculated: [number, string] = [60, 'u16'];
export const SOC_BMS: [number, string] = [62, 'u16'];
export const SOC_Display: [number, string] = [64, 'u16'];
export const batterySOH: [number, string] = [66, 'u16'];
export const CellVoltMax_mV: [number, string] = [68, 'u16'];
export const CellVoltMin_mV: [number, string] = [70, 'u16'];
export const allowedDischargePower: [number, string] = [72, 'u16'];
export const allowedChargePower: [number, string] = [74, 'u16'];
export const batteryVoltage: [number, string] = [76, 'u16'];
export const inverterVoltageFrameHigh: [number, string] = [78, 'u16'];
export const inverterVoltage: [number, string] = [80, 'u16'];
export const cellvoltages_mv: [number, string, number] = [82, 'u16', 98];
export const leadAcidBatteryVoltage: [number, string] = [278, 'u16'];
export const batteryAmps: [number, string] = [280, 'i16'];
export const temperatureMax: [number, string] = [282, 'i16'];
export const temperatureMin: [number, string] = [284, 'i16'];
export const poll_data_pid: [number, string] = [286, 'u8'];
export const open_state: [number, string] = [287, 'u8'];
export const pid_reply: [number, string] = [288, 'u16'];
export const holdPidCounter: [number, string] = [290, 'b'];
export const CellVmaxNo: [number, string] = [291, 'u8'];
export const CellVminNo: [number, string] = [292, 'u8'];
export const batteryManagementMode: [number, string] = [293, 'u8'];
export const BMS_ign: [number, string] = [294, 'u8'];
export const batteryRelay: [number, string] = [295, 'u8'];
export const waterleakageSensor: [number, string] = [296, 'u8'];
export const counter_200: [number, string] = [297, 'u8'];
export const temperature_water_inlet: [number, string] = [298, 'i8'];
export const heatertemp: [number, string] = [299, 'i8'];
export const powerRelayTemperature: [number, string] = [300, 'i8'];
export const startedUp: [number, string] = [301, 'b'];
export const ecu_serial_number: [number, string, number] = [302, 'u8', 16];
export const ecu_version_number: [number, string, number] = [318, 'u8', 16];
export const cumulative_charge_current_ah: [number, string] = [336, 'u32'];
export const cumulative_discharge_current_ah: [number, string] = [340, 'u32'];
export const cumulative_energy_charged_kWh: [number, string] = [344, 'u32'];
export const cumulative_energy_discharged_HIGH_BYTE: [number, string] = [348, 'u16'];
export const cumulative_energy_discharged_kWh: [number, string] = [352, 'u32'];
export const powered_on_total_time: [number, string] = [356, 'u32'];
export const isolation_resistance_kOhm: [number, string] = [360, 'u16'];
export const number_of_standard_charging_sessions: [number, string] = [362, 'u16'];
export const number_of_fastcharging_sessions: [number, string] = [364, 'u16'];
export const accumulated_normal_charging_energy_kWh: [number, string] = [366, 'u16'];
export const accumulated_fastcharging_energy_kWh: [number, string] = [368, 'u16'];
export const KIA_HYUNDAI_200: [number, string] = [376, 'CAN_frame'];
export const KIA_HYUNDAI_523: [number, string] = [448, 'CAN_frame'];
export const KIA_HYUNDAI_524: [number, string] = [520, 'CAN_frame'];
export const KIA64_553: [number, string] = [592, 'CAN_frame'];
export const KIA64_57F: [number, string] = [664, 'CAN_frame'];
export const KIA64_2A1: [number, string] = [736, 'CAN_frame'];
export const KIA64_7E4_OPEN_CONTACTOR_SEQUENCE: [number, string] = [808, 'CAN_frame'];
export const KIA64_7E4_poll: [number, string] = [880, 'CAN_frame'];
export const KIA64_7E4_ack: [number, string] = [952, 'CAN_frame'];
export const KIA64_CLEAR_DTC: [number, string] = [1024, 'CAN_frame'];
