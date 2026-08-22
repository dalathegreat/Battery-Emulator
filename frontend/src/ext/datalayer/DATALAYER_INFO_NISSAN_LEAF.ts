export const DATALAYER_INFO_NISSAN_LEAF_FIELDS: ([string, string] | [string, string, number])[] = [
  ['CryptoChallenge', 'u32'],
  ['SolvedChallengeMSB', 'u32'],
  ['SolvedChallengeLSB', 'u32'],
  ['GIDS', 'u16'],
  ['ChargePowerLimit', 'u16'],
  ['battery_HX_pptt', 'u16'],
  ['Insulation', 'u16'],
  ['ChargeCountQC', 'u16'],
  ['ChargeCountL1L2', 'u16'],
  ['MaxPowerForCharger', 'i16'],
  ['temperature1', 'i16'],
  ['temperature2', 'i16'],
  ['temperature3', 'i16'],
  ['temperature4', 'i16'],
  ['LEAF_gen', 'u8'],
  ['RelayCutRequest', 'u8'],
  ['FailsafeStatus', 'u8'],
  ['Interlock', 'b'],
  ['Full', 'b'],
  ['Empty', 'b'],
  ['MainRelayOn', 'b'],
  ['HeatExist', 'b'],
  ['HeatingStop', 'b'],
  ['HeatingStart', 'b'],
  ['HeaterSendRequest', 'b'],
  ['challengeFailed', 'b'],
  ['BatterySerialNumber', 'u8', 15],
  ['BatteryPartNumber', 'u8', 7],
];

export const CryptoChallenge: [number, string] = [0, 'u32'];
export const SolvedChallengeMSB: [number, string] = [4, 'u32'];
export const SolvedChallengeLSB: [number, string] = [8, 'u32'];
export const GIDS: [number, string] = [12, 'u16'];
export const ChargePowerLimit: [number, string] = [14, 'u16'];
export const battery_HX_pptt: [number, string] = [16, 'u16'];
export const Insulation: [number, string] = [18, 'u16'];
export const ChargeCountQC: [number, string] = [20, 'u16'];
export const ChargeCountL1L2: [number, string] = [22, 'u16'];
export const MaxPowerForCharger: [number, string] = [24, 'i16'];
export const temperature1: [number, string] = [26, 'i16'];
export const temperature2: [number, string] = [28, 'i16'];
export const temperature3: [number, string] = [30, 'i16'];
export const temperature4: [number, string] = [32, 'i16'];
export const LEAF_gen: [number, string] = [34, 'u8'];
export const RelayCutRequest: [number, string] = [35, 'u8'];
export const FailsafeStatus: [number, string] = [36, 'u8'];
export const Interlock: [number, string] = [37, 'b'];
export const Full: [number, string] = [38, 'b'];
export const Empty: [number, string] = [39, 'b'];
export const MainRelayOn: [number, string] = [40, 'b'];
export const HeatExist: [number, string] = [41, 'b'];
export const HeatingStop: [number, string] = [42, 'b'];
export const HeatingStart: [number, string] = [43, 'b'];
export const HeaterSendRequest: [number, string] = [44, 'b'];
export const challengeFailed: [number, string] = [45, 'b'];
export const BatterySerialNumber: [number, string, number] = [46, 'u8', 15];
export const BatteryPartNumber: [number, string, number] = [61, 'u8', 7];
