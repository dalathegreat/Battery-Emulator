export const DATALAYER_INFO_VOLVO_HYBRID_FIELDS: ([string, string] | [string, string, number])[] = [
  ['soc_bms', 'u16'],
  ['soc_calc', 'u16'],
  ['soc_rescaled', 'u16'],
  ['soh_bms', 'u16'],
  ['BECMsupplyVoltage', 'u16'],
  ['BECMBatteryVoltage', 'u16'],
  ['BECMBatteryCurrent', 'u16'],
  ['BECMUDynMaxLim', 'u16'],
  ['BECMUDynMinLim', 'u16'],
  ['HvBattPwrLimDcha1', 'u16'],
  ['HvBattPwrLimDchaSoft', 'u16'],
  ['HVSysRlySts', 'u8'],
  ['HVSysDCRlySts1', 'u8'],
  ['HVSysDCRlySts2', 'u8'],
  ['HVSysIsoRMonrSts', 'u8'],
  ['UserRequestDTCreset', 'b'],
  ['UserRequestDTCreadout', 'b'],
  ['UserRequestBECMecuReset', 'b'],
];

export const soc_bms: [number, string] = [0, 'u16'];
export const soc_calc: [number, string] = [2, 'u16'];
export const soc_rescaled: [number, string] = [4, 'u16'];
export const soh_bms: [number, string] = [6, 'u16'];
export const BECMsupplyVoltage: [number, string] = [8, 'u16'];
export const BECMBatteryVoltage: [number, string] = [10, 'u16'];
export const BECMBatteryCurrent: [number, string] = [12, 'u16'];
export const BECMUDynMaxLim: [number, string] = [14, 'u16'];
export const BECMUDynMinLim: [number, string] = [16, 'u16'];
export const HvBattPwrLimDcha1: [number, string] = [18, 'u16'];
export const HvBattPwrLimDchaSoft: [number, string] = [20, 'u16'];
export const HVSysRlySts: [number, string] = [22, 'u8'];
export const HVSysDCRlySts1: [number, string] = [23, 'u8'];
export const HVSysDCRlySts2: [number, string] = [24, 'u8'];
export const HVSysIsoRMonrSts: [number, string] = [25, 'u8'];
export const UserRequestDTCreset: [number, string] = [26, 'b'];
export const UserRequestDTCreadout: [number, string] = [27, 'b'];
export const UserRequestBECMecuReset: [number, string] = [28, 'b'];
