export const DATALAYER_INFO_BMWPHEV_FIELDS: ([string, string] | [string, string, number])[] = [
  ['min_cell_voltage_data_age', 'u64'],
  ['max_cell_voltage_data_age', 'u64'],
  ['min_soh_state', 'u16'],
  ['max_soh_state', 'u16'],
  ['iso_safety_int_kohm', 'u16'],
  ['iso_safety_ext_kohm', 'u16'],
  ['iso_safety_trg_kohm', 'u16'],
  ['iso_safety_kohm', 'u16'],
  ['battery_voltage_after_contactor', 'i16'],
  ['allowable_charge_amps', 'i16'],
  ['allowable_discharge_amps', 'i16'],
  ['ST_iso_ext', 'u8'],
  ['ST_iso_int', 'u8'],
  ['ST_valve_cooling', 'u8'],
  ['ST_interlock', 'u8'],
  ['ST_precharge', 'u8'],
  ['ST_DCSW', 'u8'],
  ['ST_EMG', 'u8'],
  ['ST_WELD', 'u8'],
  ['ST_isolation', 'u8'],
  ['ST_cold_shutoff_valve', 'u8'],
  ['battery_request_open_contactors', 'u8'],
  ['battery_request_open_contactors_instantly', 'u8'],
  ['battery_request_open_contactors_fast', 'u8'],
  ['battery_charging_condition_delta', 'u8'],
  ['dtc_count', 'u8'],
  ['iso_safety_ext_plausible', 'u8'],
  ['iso_safety_int_plausible', 'u8'],
  ['iso_safety_trg_plausible', 'u8'],
  ['iso_safety_kohm_quality', 'u8'],
  ['balancing_status', 'u8'],
  ['dtc_read_failed', 'b'],
  ['UserRequestDTCreset', 'b'],
  ['UserRequestBMSReset', 'b'],
  ['UserRequestIsolationTest', 'b'],
];

export const min_cell_voltage_data_age: [number, string] = [0, 'u64'];
export const max_cell_voltage_data_age: [number, string] = [8, 'u64'];
export const min_soh_state: [number, string] = [16, 'u16'];
export const max_soh_state: [number, string] = [18, 'u16'];
export const iso_safety_int_kohm: [number, string] = [20, 'u16'];
export const iso_safety_ext_kohm: [number, string] = [22, 'u16'];
export const iso_safety_trg_kohm: [number, string] = [24, 'u16'];
export const iso_safety_kohm: [number, string] = [26, 'u16'];
export const battery_voltage_after_contactor: [number, string] = [28, 'i16'];
export const allowable_charge_amps: [number, string] = [30, 'i16'];
export const allowable_discharge_amps: [number, string] = [32, 'i16'];
export const ST_iso_ext: [number, string] = [34, 'u8'];
export const ST_iso_int: [number, string] = [35, 'u8'];
export const ST_valve_cooling: [number, string] = [36, 'u8'];
export const ST_interlock: [number, string] = [37, 'u8'];
export const ST_precharge: [number, string] = [38, 'u8'];
export const ST_DCSW: [number, string] = [39, 'u8'];
export const ST_EMG: [number, string] = [40, 'u8'];
export const ST_WELD: [number, string] = [41, 'u8'];
export const ST_isolation: [number, string] = [42, 'u8'];
export const ST_cold_shutoff_valve: [number, string] = [43, 'u8'];
export const battery_request_open_contactors: [number, string] = [44, 'u8'];
export const battery_request_open_contactors_instantly: [number, string] = [45, 'u8'];
export const battery_request_open_contactors_fast: [number, string] = [46, 'u8'];
export const battery_charging_condition_delta: [number, string] = [47, 'u8'];
export const dtc_count: [number, string] = [48, 'u8'];
export const iso_safety_ext_plausible: [number, string] = [49, 'u8'];
export const iso_safety_int_plausible: [number, string] = [50, 'u8'];
export const iso_safety_trg_plausible: [number, string] = [51, 'u8'];
export const iso_safety_kohm_quality: [number, string] = [52, 'u8'];
export const balancing_status: [number, string] = [53, 'u8'];
export const dtc_read_failed: [number, string] = [54, 'b'];
export const UserRequestDTCreset: [number, string] = [55, 'b'];
export const UserRequestBMSReset: [number, string] = [56, 'b'];
export const UserRequestIsolationTest: [number, string] = [57, 'b'];
