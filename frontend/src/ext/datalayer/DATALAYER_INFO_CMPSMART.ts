export const DATALAYER_INFO_CMPSMART_FIELDS: ([string, string] | [string, string, number])[] = [
  ['battery_negative_contactor_state', 'u8'],
  ['battery_precharge_contactor_state', 'u8'],
  ['battery_positive_contactor_state', 'u8'],
  ['battery_state', 'u8'],
  ['eplug_status', 'u8'],
  ['HVIL_status', 'u8'],
  ['ev_warning', 'u8'],
  ['insulation_fault', 'u8'],
  ['insulation_circuit_status', 'u8'],
  ['hardware_fault_status', 'u8'],
  ['l3_fault', 'u8'],
  ['plausibility_error', 'u8'],
  ['battery_charging_status', 'u8'],
  ['battery_fault', 'u8'],
  ['hvbat_wakeup_state', 'u8'],
  ['active_DTC_code', 'u8'],
  ['alert_frame3', 'u8'],
  ['alert_frame4', 'u8'],
  ['rcd_line_active', 'b'],
  ['power_auth', 'b'],
  ['battery_balancing_active', 'b'],
  ['UserRequestDTCreset', 'b'],
];

export const battery_negative_contactor_state: [number, string] = [0, 'u8'];
export const battery_precharge_contactor_state: [number, string] = [1, 'u8'];
export const battery_positive_contactor_state: [number, string] = [2, 'u8'];
export const battery_state: [number, string] = [3, 'u8'];
export const eplug_status: [number, string] = [4, 'u8'];
export const HVIL_status: [number, string] = [5, 'u8'];
export const ev_warning: [number, string] = [6, 'u8'];
export const insulation_fault: [number, string] = [7, 'u8'];
export const insulation_circuit_status: [number, string] = [8, 'u8'];
export const hardware_fault_status: [number, string] = [9, 'u8'];
export const l3_fault: [number, string] = [10, 'u8'];
export const plausibility_error: [number, string] = [11, 'u8'];
export const battery_charging_status: [number, string] = [12, 'u8'];
export const battery_fault: [number, string] = [13, 'u8'];
export const hvbat_wakeup_state: [number, string] = [14, 'u8'];
export const active_DTC_code: [number, string] = [15, 'u8'];
export const alert_frame3: [number, string] = [16, 'u8'];
export const alert_frame4: [number, string] = [17, 'u8'];
export const rcd_line_active: [number, string] = [18, 'b'];
export const power_auth: [number, string] = [19, 'b'];
export const battery_balancing_active: [number, string] = [20, 'b'];
export const UserRequestDTCreset: [number, string] = [21, 'b'];
