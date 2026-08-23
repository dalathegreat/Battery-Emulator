export const DATALAYER_INFO_MEB_FIELDS: ([string, string] | [string, string, number])[] = [
  ['isolation_resistance', 'u32'],
  ['BMS_voltage_intermediate_dV', 'i32'],
  ['BMS_voltage_dV', 'i32'],
  ['battery_temperature_dC', 'u16'],
  ['rt_overcurrent', 'u8'],
  ['rt_CAN_fault', 'u8'],
  ['rt_overcharge', 'u8'],
  ['rt_SOC_high', 'u8'],
  ['rt_SOC_low', 'u8'],
  ['rt_SOC_jumping', 'u8'],
  ['rt_temp_difference', 'u8'],
  ['rt_cell_overtemp', 'u8'],
  ['rt_cell_undertemp', 'u8'],
  ['rt_battery_overvolt', 'u8'],
  ['rt_battery_undervol', 'u8'],
  ['rt_cell_overvolt', 'u8'],
  ['rt_cell_undervol', 'u8'],
  ['rt_cell_imbalance', 'u8'],
  ['rt_battery_unathorized', 'u8'],
  ['HVIL', 'u8'],
  ['BMS_mode', 'u8'],
  ['battery_diagnostic', 'u8'],
  ['status_HV_PTC_line', 'u8'],
  ['warning_support', 'u8'],
  ['BMS_status_voltage_free', 'u8'],
  ['BMS_error_status', 'u8'],
  ['BMS_Kl30c_Status', 'u8'],
  ['balancing_active', 'u8'],
  ['BMS_welded_contactors_status', 'u8'],
  ['balancing_request', 'b'],
  ['charging_active', 'b'],
  ['BMS_OBD_MIL', 'b'],
  ['BMS_error_lamp_req', 'b'],
  ['BMS_warning_lamp_req', 'b'],
  ['BMS_fault_performance', 'b'],
  ['BMS_fault_emergency_shutdown_crash', 'b'],
  ['BMS_error_shutdown_request', 'b'],
  ['BMS_error_shutdown', 'b'],
  ['SDSW', 'b'],
  ['pilotline', 'b'],
  ['transportmode', 'b'],
  ['componentprotection', 'b'],
  ['shutdown_active', 'b'],
  ['battery_heating', 'b'],
  ['', ' ', 2],
  ['temp_points', 'f', 18],
  ['celltemperature_dC', 'i16', 56],
  ['dtc_read_in_progress', 'b'],
  ['UserRequestDTCreset', 'b'],
  ['UserRequestDTCreadout', 'b'],
  ['UserRequestCrashReset', 'b'],
  ['UserRequestBMSReset', 'b'],
];

export const isolation_resistance: [number, string] = [0, 'u32'];
export const BMS_voltage_intermediate_dV: [number, string] = [4, 'i32'];
export const BMS_voltage_dV: [number, string] = [8, 'i32'];
export const battery_temperature_dC: [number, string] = [12, 'u16'];
export const rt_overcurrent: [number, string] = [14, 'u8'];
export const rt_CAN_fault: [number, string] = [15, 'u8'];
export const rt_overcharge: [number, string] = [16, 'u8'];
export const rt_SOC_high: [number, string] = [17, 'u8'];
export const rt_SOC_low: [number, string] = [18, 'u8'];
export const rt_SOC_jumping: [number, string] = [19, 'u8'];
export const rt_temp_difference: [number, string] = [20, 'u8'];
export const rt_cell_overtemp: [number, string] = [21, 'u8'];
export const rt_cell_undertemp: [number, string] = [22, 'u8'];
export const rt_battery_overvolt: [number, string] = [23, 'u8'];
export const rt_battery_undervol: [number, string] = [24, 'u8'];
export const rt_cell_overvolt: [number, string] = [25, 'u8'];
export const rt_cell_undervol: [number, string] = [26, 'u8'];
export const rt_cell_imbalance: [number, string] = [27, 'u8'];
export const rt_battery_unathorized: [number, string] = [28, 'u8'];
export const HVIL: [number, string] = [29, 'u8'];
export const BMS_mode: [number, string] = [30, 'u8'];
export const battery_diagnostic: [number, string] = [31, 'u8'];
export const status_HV_PTC_line: [number, string] = [32, 'u8'];
export const warning_support: [number, string] = [33, 'u8'];
export const BMS_status_voltage_free: [number, string] = [34, 'u8'];
export const BMS_error_status: [number, string] = [35, 'u8'];
export const BMS_Kl30c_Status: [number, string] = [36, 'u8'];
export const balancing_active: [number, string] = [37, 'u8'];
export const BMS_welded_contactors_status: [number, string] = [38, 'u8'];
export const balancing_request: [number, string] = [39, 'b'];
export const charging_active: [number, string] = [40, 'b'];
export const BMS_OBD_MIL: [number, string] = [41, 'b'];
export const BMS_error_lamp_req: [number, string] = [42, 'b'];
export const BMS_warning_lamp_req: [number, string] = [43, 'b'];
export const BMS_fault_performance: [number, string] = [44, 'b'];
export const BMS_fault_emergency_shutdown_crash: [number, string] = [45, 'b'];
export const BMS_error_shutdown_request: [number, string] = [46, 'b'];
export const BMS_error_shutdown: [number, string] = [47, 'b'];
export const SDSW: [number, string] = [48, 'b'];
export const pilotline: [number, string] = [49, 'b'];
export const transportmode: [number, string] = [50, 'b'];
export const componentprotection: [number, string] = [51, 'b'];
export const shutdown_active: [number, string] = [52, 'b'];
export const battery_heating: [number, string] = [53, 'b'];
export const temp_points: [number, string, number] = [56, 'f', 18];
export const celltemperature_dC: [number, string, number] = [128, 'i16', 56];
export const dtc_read_in_progress: [number, string] = [240, 'b'];
export const UserRequestDTCreset: [number, string] = [241, 'b'];
export const UserRequestDTCreadout: [number, string] = [242, 'b'];
export const UserRequestCrashReset: [number, string] = [243, 'b'];
export const UserRequestBMSReset: [number, string] = [244, 'b'];
