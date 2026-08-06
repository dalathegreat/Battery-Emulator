// GeelySeaBattery: 1096 bytes; base classes: CanBattery@0
export const GEELY_SEA_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['renderer', 'GeelySeaHtmlRenderer'],
  ['', ' ', 4],
  ['previousMillis20', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis1000', 'u32'],
  ['previousMillisWakeup', 'u32'],
  ['pause_polling_seconds', 'u8'],
  ['DTC_readout_in_progress', 'b'],
  ['poll_commands', '9bff'],
  ['', ' ', 24],
  ['poll_index', 'u8'],
  ['', ' '],
  ['currentpoll', 'u16'],
  ['reply_poll', 'u16'],
  ['battery_alive', 'b'],
  ['', ' '],
  ['pack_current_dA', 'i16'],
  ['pack_voltage_dV', 'u16'],
  ['', ' ', 6],
  ['SEA_536', 'CAN_frame'],
  ['SEA_060', 'CAN_frame'],
  ['SEA_156', 'CAN_frame'],
  ['SEA_171', 'CAN_frame'],
  ['SEA_218', 'CAN_frame'],
  ['SEA_490', 'CAN_frame'],
  ['SEA_103', 'CAN_frame'],
  ['SEA_Polling_Req', 'CAN_frame'],
  ['SEA_DTC_Req', 'CAN_frame'],
  ['SEA_Flowcontrol', 'CAN_frame'],
  ['SEA_DTC_Erase', 'CAN_frame'],
  ['SEA_StartDiag', 'CAN_frame'],
  ['SEA_ClearCrash', 'CAN_frame'],
  ['SEA_BECM_ECUreset', 'CAN_frame'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const renderer: [number, string] = [24, 'GeelySeaHtmlRenderer'];
export const previousMillis20: [number, string] = [28, 'u32'];
export const previousMillis100: [number, string] = [32, 'u32'];
export const previousMillis1000: [number, string] = [36, 'u32'];
export const previousMillisWakeup: [number, string] = [40, 'u32'];
export const pause_polling_seconds: [number, string] = [44, 'u8'];
export const DTC_readout_in_progress: [number, string] = [45, 'b'];
export const poll_commands: [number, string] = [46, '9bff'];
export const poll_index: [number, string] = [70, 'u8'];
export const currentpoll: [number, string] = [72, 'u16'];
export const reply_poll: [number, string] = [74, 'u16'];
export const battery_alive: [number, string] = [76, 'b'];
export const pack_current_dA: [number, string] = [78, 'i16'];
export const pack_voltage_dV: [number, string] = [80, 'u16'];
export const SEA_536: [number, string] = [88, 'CAN_frame'];
export const SEA_060: [number, string] = [160, 'CAN_frame'];
export const SEA_156: [number, string] = [232, 'CAN_frame'];
export const SEA_171: [number, string] = [304, 'CAN_frame'];
export const SEA_218: [number, string] = [376, 'CAN_frame'];
export const SEA_490: [number, string] = [448, 'CAN_frame'];
export const SEA_103: [number, string] = [520, 'CAN_frame'];
export const SEA_Polling_Req: [number, string] = [592, 'CAN_frame'];
export const SEA_DTC_Req: [number, string] = [664, 'CAN_frame'];
export const SEA_Flowcontrol: [number, string] = [736, 'CAN_frame'];
export const SEA_DTC_Erase: [number, string] = [808, 'CAN_frame'];
export const SEA_StartDiag: [number, string] = [880, 'CAN_frame'];
export const SEA_ClearCrash: [number, string] = [952, 'CAN_frame'];
export const SEA_BECM_ECUreset: [number, string] = [1024, 'CAN_frame'];
