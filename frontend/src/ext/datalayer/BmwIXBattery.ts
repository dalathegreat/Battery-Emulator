// BmwIXBattery: 856 bytes; base classes: CanBattery@0
export const BMW_IXBATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['previous', 'b'],
  ['closed', 'b'],
  ['previous_2', 'b'],
  ['present', 'b'],
  ['open', 'b'],
  ['present_2', 'b'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['userRequestContactorClose', 'b'],
  ['userRequestContactorOpen', 'b'],
  ['UserRequestDTCreset', 'b'],
  ['UserRequestBMSReset', 'b'],
  ['UserRequestDTCRead', 'b'],
  ['UserRequestEnergySavingModeReset', 'b'],
  ['startup_reset_complete', 'b'],
  ['renderer', 'BmwIXHtmlRenderer'],
  ['', ' ', 7],
  ['previousMillis10', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis1000', 'u32'],
  ['previousMillis10000', 'u32'],
  ['min_cell_voltage_lastchanged', 'u32'],
  ['max_cell_voltage_lastchanged', 'u32'],
  ['cmdState', 'CmdState'],
  ['battery_awake', 'b'],
  ['contactorCloseReq', 'b'],
  ['ContactorCloseRequest', 'ContactorCloseRequestStruct'],
  ['ContactorState', 'ContactorStateStruct'],
  ['InverterContactorCloseRequest', 'InverterContactorCloseRequestStruct'],
  ['', ' ', 4],
  ['BMWiX_16E', 'CAN_frame'],
  ['BMWiX_510', 'CAN_frame'],
  ['battery_info_available', 'b'],
  ['', ' ', 3],
  ['battery_serial_number', 'u32'],
  ['battery_current', 'i32'],
  ['battery_voltage', 'u16'],
  ['terminal30_12v_voltage', 'u16'],
  ['battery_voltage_after_contactor', 'u16'],
  ['min_soc_state', 'u16'],
  ['avg_soc_state', 'u16'],
  ['max_soc_state', 'u16'],
  ['min_soh_state', 'u16'],
  ['avg_soh_state', 'u16'],
  ['max_soh_state', 'u16'],
  ['max_design_voltage', 'u16'],
  ['min_design_voltage', 'u16'],
  ['', ' ', 2],
  ['remaining_capacity', 'u32'],
  ['max_capacity', 'u32'],
  ['min_battery_temperature', 'i16'],
  ['avg_battery_temperature', 'i16'],
  ['max_battery_temperature', 'i16'],
  ['main_contactor_temperature', 'i16'],
  ['min_cell_voltage', 'u16'],
  ['max_cell_voltage', 'u16'],
  ['sme_uptime', 'u32'],
  ['allowable_charge_amps', 'i16'],
  ['allowable_discharge_amps', 'i16'],
  ['iso_safety_positive', 'i32'],
  ['iso_safety_negative', 'i32'],
  ['iso_safety_parallel', 'i32'],
  ['count_full_charges', 'i16'],
  ['count_charges', 'i16'],
  ['hvil_status', 'i16'],
  ['voltage_qualifier_status', 'i16'],
  ['balancing_status', 'i16'],
  ['energy_saving_mode_status', 'i16'],
  ['contactors_closed', 'u8'],
  ['contactor_status_precharge', 'u8'],
  ['contactor_status_negative', 'u8'],
  ['contactor_status_positive', 'u8'],
  ['pyro_status_pss1', 'u8'],
  ['pyro_status_pss4', 'u8'],
  ['pyro_status_pss6', 'u8'],
  ['uds_req_id_counter', 'u8'],
  ['uds_req_id_counter_slow', 'u8'],
  ['detected_number_of_cells', 'u8'],
  ['', ' ', 2],
  ['gUDSContext', 'UDS_CONTEXT'],
  ['counter_10ms', 'u16'],
  ['counter_100ms', 'u8'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const previous: [number, string] = [0, 'b'];
export const closed: [number, string] = [0, 'b'];
export const previous_2: [number, string] = [0, 'b'];
export const present: [number, string] = [1, 'b'];
export const open: [number, string] = [1, 'b'];
export const present_2: [number, string] = [1, 'b'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const userRequestContactorClose: [number, string] = [24, 'b'];
export const userRequestContactorOpen: [number, string] = [25, 'b'];
export const UserRequestDTCreset: [number, string] = [26, 'b'];
export const UserRequestBMSReset: [number, string] = [27, 'b'];
export const UserRequestDTCRead: [number, string] = [28, 'b'];
export const UserRequestEnergySavingModeReset: [number, string] = [29, 'b'];
export const startup_reset_complete: [number, string] = [30, 'b'];
export const renderer: [number, string] = [32, 'BmwIXHtmlRenderer'];
export const previousMillis10: [number, string] = [40, 'u32'];
export const previousMillis100: [number, string] = [44, 'u32'];
export const previousMillis1000: [number, string] = [48, 'u32'];
export const previousMillis10000: [number, string] = [52, 'u32'];
export const min_cell_voltage_lastchanged: [number, string] = [56, 'u32'];
export const max_cell_voltage_lastchanged: [number, string] = [60, 'u32'];
export const cmdState: [number, string] = [64, 'CmdState'];
export const battery_awake: [number, string] = [68, 'b'];
export const contactorCloseReq: [number, string] = [69, 'b'];
export const ContactorCloseRequest: [number, string] = [70, 'ContactorCloseRequestStruct'];
export const ContactorState: [number, string] = [72, 'ContactorStateStruct'];
export const InverterContactorCloseRequest: [number, string] = [74, 'InverterContactorCloseRequestStruct'];
export const BMWiX_16E: [number, string] = [80, 'CAN_frame'];
export const BMWiX_510: [number, string] = [152, 'CAN_frame'];
export const battery_info_available: [number, string] = [224, 'b'];
export const battery_serial_number: [number, string] = [228, 'u32'];
export const battery_current: [number, string] = [232, 'i32'];
export const battery_voltage: [number, string] = [236, 'u16'];
export const terminal30_12v_voltage: [number, string] = [238, 'u16'];
export const battery_voltage_after_contactor: [number, string] = [240, 'u16'];
export const min_soc_state: [number, string] = [242, 'u16'];
export const avg_soc_state: [number, string] = [244, 'u16'];
export const max_soc_state: [number, string] = [246, 'u16'];
export const min_soh_state: [number, string] = [248, 'u16'];
export const avg_soh_state: [number, string] = [250, 'u16'];
export const max_soh_state: [number, string] = [252, 'u16'];
export const max_design_voltage: [number, string] = [254, 'u16'];
export const min_design_voltage: [number, string] = [256, 'u16'];
export const remaining_capacity: [number, string] = [260, 'u32'];
export const max_capacity: [number, string] = [264, 'u32'];
export const min_battery_temperature: [number, string] = [268, 'i16'];
export const avg_battery_temperature: [number, string] = [270, 'i16'];
export const max_battery_temperature: [number, string] = [272, 'i16'];
export const main_contactor_temperature: [number, string] = [274, 'i16'];
export const min_cell_voltage: [number, string] = [276, 'u16'];
export const max_cell_voltage: [number, string] = [278, 'u16'];
export const sme_uptime: [number, string] = [280, 'u32'];
export const allowable_charge_amps: [number, string] = [284, 'i16'];
export const allowable_discharge_amps: [number, string] = [286, 'i16'];
export const iso_safety_positive: [number, string] = [288, 'i32'];
export const iso_safety_negative: [number, string] = [292, 'i32'];
export const iso_safety_parallel: [number, string] = [296, 'i32'];
export const count_full_charges: [number, string] = [300, 'i16'];
export const count_charges: [number, string] = [302, 'i16'];
export const hvil_status: [number, string] = [304, 'i16'];
export const voltage_qualifier_status: [number, string] = [306, 'i16'];
export const balancing_status: [number, string] = [308, 'i16'];
export const energy_saving_mode_status: [number, string] = [310, 'i16'];
export const contactors_closed: [number, string] = [312, 'u8'];
export const contactor_status_precharge: [number, string] = [313, 'u8'];
export const contactor_status_negative: [number, string] = [314, 'u8'];
export const contactor_status_positive: [number, string] = [315, 'u8'];
export const pyro_status_pss1: [number, string] = [316, 'u8'];
export const pyro_status_pss4: [number, string] = [317, 'u8'];
export const pyro_status_pss6: [number, string] = [318, 'u8'];
export const uds_req_id_counter: [number, string] = [319, 'u8'];
export const uds_req_id_counter_slow: [number, string] = [320, 'u8'];
export const detected_number_of_cells: [number, string] = [321, 'u8'];
export const gUDSContext: [number, string] = [324, 'UDS_CONTEXT'];
export const counter_10ms: [number, string] = [848, 'u16'];
export const counter_100ms: [number, string] = [850, 'u8'];
