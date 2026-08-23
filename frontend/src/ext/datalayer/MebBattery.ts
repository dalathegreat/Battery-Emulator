// MebBattery: 2704 bytes; base classes: CanBattery@0, IsoTp@24
export const MEB_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['_idx', 'i32'],
  ['_bs', 'u8'],
  ['__vptr$IsoTp', '__vtbl_ptr_type*'],
  ['_stmin', 'u8'],
  ['_len', 'i32'],
  ['__rx', 'TpCon'],
  ['_state', 'i32'],
  ['_bs_2', 'u8'],
  ['_sn', 'u8'],
  ['_buf', 'u8', 512],
  ['__tx', 'TpCon'],
  ['__rxfc', 'FcOpts'],
  ['__txfc', 'FcOpts'],
  ['__tx_wft', 'u8'],
  ['__tatype', 'isotp_tatype'],
  ['__rxtimer', 'u32'],
  ['__txtimer', 'u32'],
  ['__tx_id', 'u32'],
  ['__addrmode', 'isotp_addrmode'],
  ['__tx_addr', 'u8'],
  ['__rx_addr', 'u8'],
  ['renderer', 'MebHtmlRenderer'],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['datalayer_meb', 'DATALAYER_INFO_MEB*'],
  ['platform', 'VAGPlatform'],
  ['previousMillis10ms', 'u32'],
  ['previousMillis20ms', 'u32'],
  ['previousMillis40ms', 'u32'],
  ['previousMillis50ms', 'u32'],
  ['previousMillis100ms', 'u32'],
  ['previousMillis200ms', 'u32'],
  ['previousMillis500ms', 'u32'],
  ['previousMillis1s', 'u32'],
  ['toggle', 'b'],
  ['counter_1000ms', 'u8'],
  ['counter_200ms', 'u8'],
  ['counter_100ms', 'u8'],
  ['counter_50ms', 'u8'],
  ['counter_40ms', 'u8'],
  ['counter_20ms', 'u8'],
  ['counter_10ms', 'u8'],
  ['counter_040', 'u8'],
  ['counter_0F7', 'u8'],
  ['counter_3b5', 'u8'],
  ['BMS_04_counter', 'u8'],
  ['BMS_07_counter', 'u8'],
  ['BMS_20_counter', 'u8'],
  ['EM1_01_counter', 'u8'],
  ['BMS_DC_01_counter', 'u8'],
  ['BMS_11_counter', 'u8'],
  ['BMS_11_CRC', 'u8'],
  ['uds_request_pending', 'b'],
  ['uds_request_timestamp', 'u32'],
  ['basic_settings_state', 'BasicSettingsState'],
  ['basic_settings_routine_id', 'u16'],
  ['basic_settings_routine_param', 'u16'],
  ['security_access_seed', 'u32'],
  ['security_login_key', 'u32'],
  ['basic_settings_wait_ms', 'u32'],
  ['bms_reset_state', 'BmsResetState'],
  ['bms_reset_ms', 'u32'],
  ['bms_reset_active', 'b'],
  ['bms_reset_tx_suppressed', 'b'],
  ['startup_bms_checked', 'b'],
  ['poll_pid', 'u32'],
  ['nof_cells_determined', 'b'],
  ['pid_reply', 'u32'],
  ['battery_soc_polled', 'u16'],
  ['battery_soh_polled', 'u16'],
  ['battery_voltage_polled', 'u16'],
  ['battery_current_polled', 'i16'],
  ['battery_max_temp', 'i16'],
  ['battery_min_temp', 'i16'],
  ['battery_max_charge_voltage', 'u16'],
  ['battery_min_discharge_voltage', 'u16'],
  ['battery_allowed_charge_power', 'u16'],
  ['battery_allowed_discharge_power', 'u16'],
  ['cellvoltages_polled', 'u16', 108],
  ['tempval', 'u16'],
  ['BMS_ext_limits_active', 'b'],
  ['BMS_mode', 'u8'],
  ['BMS_HVIL_status', 'u8'],
  ['BMS_fault_performance', 'b'],
  ['BMS_current', 'u16'],
  ['BMS_fault_emergency_shutdown_crash', 'b'],
  ['BMS_voltage_intermediate', 'u32'],
  ['BMS_voltage', 'u32'],
  ['BMS_usable_batt_energy_Wh', 'i32'],
  ['BMS_usable_batt_energy_t_Wh', 'i32'],
  ['BMS_max_usable_batt_energy_Wh', 'i32'],
  ['BMS_nominal_voltage_dV', 'u16'],
  ['BMS_status_voltage_free', 'u8'],
  ['BMS_OBD_MIL', 'b'],
  ['BMS_error_status', 'u8'],
  ['BMS_capacity_ah', 'u16'],
  ['BMS_error_lamp_req', 'b'],
  ['BMS_warning_lamp_req', 'b'],
  ['BMS_Kl30c_Status', 'u8'],
  ['service_disconnect_switch_missing', 'b'],
  ['pilotline_open', 'b'],
  ['balancing_request', 'b'],
  ['battery_diagnostic', 'u8'],
  ['battery_Wh_left', 'u16'],
  ['battery_Wh_max', 'u16'],
  ['battery_potential_status', 'u8'],
  ['battery_temperature_warning', 'u8'],
  ['max_discharge_power_watt', 'u16'],
  ['max_discharge_current_amp', 'u16'],
  ['max_charge_power_watt', 'u16'],
  ['max_charge_current_amp', 'u16'],
  ['battery_SOC', 'u16'],
  ['usable_energy_amount_Wh', 'u16'],
  ['status_HV_PTC_line', 'u8'],
  ['warning_support', 'u8'],
  ['battery_heating_active', 'b'],
  ['power_discharge_percentage', 'u16'],
  ['power_charge_percentage', 'u16'],
  ['actual_battery_voltage', 'u16'],
  ['regen_battery', 'u16'],
  ['energy_extracted_from_battery', 'u16'],
  ['max_fastcharging_current_amp', 'u16'],
  ['BMS_Status_DCLS', 'u8'],
  ['DC_voltage_DCLS', 'u16'],
  ['DC_voltage_chargeport', 'u16'],
  ['BMS_welded_contactors_status', 'u8'],
  ['BMS_error_shutdown_request', 'b'],
  ['BMS_error_shutdown', 'b'],
  ['power_battery_heating_watt', 'u16'],
  ['power_battery_heating_req_watt', 'u16'],
  ['cooling_request', 'u8'],
  ['heating_request', 'u8'],
  ['balancing_active', 'u8'],
  ['charging_active', 'b'],
  ['max_energy_Wh', 'u16'],
  ['max_charge_percent', 'u16'],
  ['min_charge_percent', 'u16'],
  ['isolation_resistance_kOhm', 'u16'],
  ['battery_heating_installed', 'b'],
  ['error_NT_circuit', 'b'],
  ['pump_1_control', 'u8'],
  ['pump_2_control', 'u8'],
  ['target_flow_temperature_C', 'u8'],
  ['return_temperature_C', 'u8'],
  ['status_valve_1', 'u8'],
  ['status_valve_2', 'u8'],
  ['temperature_request', 'u8'],
  ['performance_index_discharge_peak_temperature_percentage', 'u16'],
  ['performance_index_charge_peak_temperature_percentage', 'u16'],
  ['temperature_status_discharge', 'u8'],
  ['temperature_status_charge', 'u8'],
  ['isolation_fault', 'u8'],
  ['isolation_status', 'u8'],
  ['actual_temperature_highest_C', 'u8'],
  ['actual_temperature_lowest_C', 'u8'],
  ['actual_cellvoltage_highest_mV', 'u16'],
  ['actual_cellvoltage_lowest_mV', 'u16'],
  ['predicted_power_dyn_standard_watt', 'u16'],
  ['predicted_time_dyn_standard_minutes', 'u8'],
  ['mux', 'u8'],
  ['duration_discharge_power_watt', 'u16'],
  ['duration_charge_power_watt', 'u16'],
  ['maximum_voltage', 'u16'],
  ['minimum_voltage', 'u16'],
  ['battery_serialnumber', 'u8', 26],
  ['realtime_overcurrent_monitor', 'u8'],
  ['realtime_CAN_communication_fault', 'u8'],
  ['realtime_overcharge_warning', 'u8'],
  ['realtime_SOC_too_high', 'u8'],
  ['realtime_SOC_too_low', 'u8'],
  ['realtime_SOC_jumping_warning', 'u8'],
  ['realtime_temperature_difference_warning', 'u8'],
  ['realtime_cell_overtemperature_warning', 'u8'],
  ['realtime_cell_undertemperature_warning', 'u8'],
  ['realtime_battery_overvoltage_warning', 'u8'],
  ['realtime_battery_undervoltage_warning', 'u8'],
  ['realtime_cell_overvoltage_warning', 'u8'],
  ['realtime_cell_undervoltage_warning', 'u8'],
  ['realtime_cell_imbalance_warning', 'u8'],
  ['realtime_warning_battery_unathorized', 'u8'],
  ['component_protection_active', 'b'],
  ['shutdown_active', 'b'],
  ['transportation_mode_active', 'b'],
  ['KL15_mode', 'u8'],
  ['bus_knockout_timer', 'u8'],
  ['hybrid_wakeup_reason', 'u8'],
  ['wakeup_type', 'u8'],
  ['instrument_cluster_request', 'b'],
  ['seconds', 'u8'],
  ['first_can_msg_timestamp', 'u32'],
  ['last_can_msg_timestamp', 'u32'],
  ['hv_requested', 'b'],
  ['kwh_charge', 'i32'],
  ['kwh_discharge', 'i32'],
  ['Airbag_01_frame', 'CAN_frame'],
  ['EM1_01_frame', 'CAN_frame'],
  ['ESC_51_Auth_frame', 'CAN_frame'],
  ['Diagnose_01_frame', 'CAN_frame'],
  ['Temperaturen_01_frame', 'CAN_frame'],
  ['Motor_Code_01_frame', 'CAN_frame'],
  ['Klemmen_Status_01_frame', 'CAN_frame'],
  ['ESP_21_frame', 'CAN_frame'],
  ['NMH_Gateway_frame', 'CAN_frame'],
  ['Motor_14_frame', 'CAN_frame'],
  ['HVLM_14_frame', 'CAN_frame'],
  ['HVK_01_frame', 'CAN_frame'],
  ['Motor_54_frame', 'CAN_frame'],
  ['Motor_EV_01_frame', 'CAN_frame'],
  ['can_msg_received', 'u32'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const _idx: [number, string] = [24, 'i32'];
export const _bs: [number, string] = [24, 'u8'];
export const __vptr$IsoTp: [number, string] = [24, '__vtbl_ptr_type*'];
export const _stmin: [number, string] = [25, 'u8'];
export const _len: [number, string] = [28, 'i32'];
export const __rx: [number, string] = [28, 'TpCon'];
export const _state: [number, string] = [32, 'i32'];
export const _bs_2: [number, string] = [36, 'u8'];
export const _sn: [number, string] = [37, 'u8'];
export const _buf: [number, string, number] = [38, 'u8', 512];
export const __tx: [number, string] = [556, 'TpCon'];
export const __rxfc: [number, string] = [1084, 'FcOpts'];
export const __txfc: [number, string] = [1086, 'FcOpts'];
export const __tx_wft: [number, string] = [1088, 'u8'];
export const __tatype: [number, string] = [1092, 'isotp_tatype'];
export const __rxtimer: [number, string] = [1096, 'u32'];
export const __txtimer: [number, string] = [1100, 'u32'];
export const __tx_id: [number, string] = [1104, 'u32'];
export const __addrmode: [number, string] = [1108, 'isotp_addrmode'];
export const __tx_addr: [number, string] = [1112, 'u8'];
export const __rx_addr: [number, string] = [1113, 'u8'];
export const renderer: [number, string] = [1116, 'MebHtmlRenderer'];
export const datalayer_battery: [number, string] = [1124, 'DATALAYER_BATTERY_TYPE*'];
export const datalayer_meb: [number, string] = [1128, 'DATALAYER_INFO_MEB*'];
export const platform: [number, string] = [1132, 'VAGPlatform'];
export const previousMillis10ms: [number, string] = [1136, 'u32'];
export const previousMillis20ms: [number, string] = [1140, 'u32'];
export const previousMillis40ms: [number, string] = [1144, 'u32'];
export const previousMillis50ms: [number, string] = [1148, 'u32'];
export const previousMillis100ms: [number, string] = [1152, 'u32'];
export const previousMillis200ms: [number, string] = [1156, 'u32'];
export const previousMillis500ms: [number, string] = [1160, 'u32'];
export const previousMillis1s: [number, string] = [1164, 'u32'];
export const toggle: [number, string] = [1168, 'b'];
export const counter_1000ms: [number, string] = [1169, 'u8'];
export const counter_200ms: [number, string] = [1170, 'u8'];
export const counter_100ms: [number, string] = [1171, 'u8'];
export const counter_50ms: [number, string] = [1172, 'u8'];
export const counter_40ms: [number, string] = [1173, 'u8'];
export const counter_20ms: [number, string] = [1174, 'u8'];
export const counter_10ms: [number, string] = [1175, 'u8'];
export const counter_040: [number, string] = [1176, 'u8'];
export const counter_0F7: [number, string] = [1177, 'u8'];
export const counter_3b5: [number, string] = [1178, 'u8'];
export const BMS_04_counter: [number, string] = [1179, 'u8'];
export const BMS_07_counter: [number, string] = [1180, 'u8'];
export const BMS_20_counter: [number, string] = [1181, 'u8'];
export const EM1_01_counter: [number, string] = [1182, 'u8'];
export const BMS_DC_01_counter: [number, string] = [1183, 'u8'];
export const BMS_11_counter: [number, string] = [1184, 'u8'];
export const BMS_11_CRC: [number, string] = [1185, 'u8'];
export const uds_request_pending: [number, string] = [1186, 'b'];
export const uds_request_timestamp: [number, string] = [1188, 'u32'];
export const basic_settings_state: [number, string] = [1192, 'BasicSettingsState'];
export const basic_settings_routine_id: [number, string] = [1194, 'u16'];
export const basic_settings_routine_param: [number, string] = [1196, 'u16'];
export const security_access_seed: [number, string] = [1200, 'u32'];
export const security_login_key: [number, string] = [1204, 'u32'];
export const basic_settings_wait_ms: [number, string] = [1208, 'u32'];
export const bms_reset_state: [number, string] = [1212, 'BmsResetState'];
export const bms_reset_ms: [number, string] = [1216, 'u32'];
export const bms_reset_active: [number, string] = [1220, 'b'];
export const bms_reset_tx_suppressed: [number, string] = [1221, 'b'];
export const startup_bms_checked: [number, string] = [1222, 'b'];
export const poll_pid: [number, string] = [1224, 'u32'];
export const nof_cells_determined: [number, string] = [1228, 'b'];
export const pid_reply: [number, string] = [1232, 'u32'];
export const battery_soc_polled: [number, string] = [1236, 'u16'];
export const battery_soh_polled: [number, string] = [1238, 'u16'];
export const battery_voltage_polled: [number, string] = [1240, 'u16'];
export const battery_current_polled: [number, string] = [1242, 'i16'];
export const battery_max_temp: [number, string] = [1244, 'i16'];
export const battery_min_temp: [number, string] = [1246, 'i16'];
export const battery_max_charge_voltage: [number, string] = [1248, 'u16'];
export const battery_min_discharge_voltage: [number, string] = [1250, 'u16'];
export const battery_allowed_charge_power: [number, string] = [1252, 'u16'];
export const battery_allowed_discharge_power: [number, string] = [1254, 'u16'];
export const cellvoltages_polled: [number, string, number] = [1256, 'u16', 108];
export const tempval: [number, string] = [1472, 'u16'];
export const BMS_ext_limits_active: [number, string] = [1474, 'b'];
export const BMS_mode: [number, string] = [1475, 'u8'];
export const BMS_HVIL_status: [number, string] = [1476, 'u8'];
export const BMS_fault_performance: [number, string] = [1477, 'b'];
export const BMS_current: [number, string] = [1478, 'u16'];
export const BMS_fault_emergency_shutdown_crash: [number, string] = [1480, 'b'];
export const BMS_voltage_intermediate: [number, string] = [1484, 'u32'];
export const BMS_voltage: [number, string] = [1488, 'u32'];
export const BMS_usable_batt_energy_Wh: [number, string] = [1492, 'i32'];
export const BMS_usable_batt_energy_t_Wh: [number, string] = [1496, 'i32'];
export const BMS_max_usable_batt_energy_Wh: [number, string] = [1500, 'i32'];
export const BMS_nominal_voltage_dV: [number, string] = [1504, 'u16'];
export const BMS_status_voltage_free: [number, string] = [1506, 'u8'];
export const BMS_OBD_MIL: [number, string] = [1507, 'b'];
export const BMS_error_status: [number, string] = [1508, 'u8'];
export const BMS_capacity_ah: [number, string] = [1510, 'u16'];
export const BMS_error_lamp_req: [number, string] = [1512, 'b'];
export const BMS_warning_lamp_req: [number, string] = [1513, 'b'];
export const BMS_Kl30c_Status: [number, string] = [1514, 'u8'];
export const service_disconnect_switch_missing: [number, string] = [1515, 'b'];
export const pilotline_open: [number, string] = [1516, 'b'];
export const balancing_request: [number, string] = [1517, 'b'];
export const battery_diagnostic: [number, string] = [1518, 'u8'];
export const battery_Wh_left: [number, string] = [1520, 'u16'];
export const battery_Wh_max: [number, string] = [1522, 'u16'];
export const battery_potential_status: [number, string] = [1524, 'u8'];
export const battery_temperature_warning: [number, string] = [1525, 'u8'];
export const max_discharge_power_watt: [number, string] = [1526, 'u16'];
export const max_discharge_current_amp: [number, string] = [1528, 'u16'];
export const max_charge_power_watt: [number, string] = [1530, 'u16'];
export const max_charge_current_amp: [number, string] = [1532, 'u16'];
export const battery_SOC: [number, string] = [1534, 'u16'];
export const usable_energy_amount_Wh: [number, string] = [1536, 'u16'];
export const status_HV_PTC_line: [number, string] = [1538, 'u8'];
export const warning_support: [number, string] = [1539, 'u8'];
export const battery_heating_active: [number, string] = [1540, 'b'];
export const power_discharge_percentage: [number, string] = [1542, 'u16'];
export const power_charge_percentage: [number, string] = [1544, 'u16'];
export const actual_battery_voltage: [number, string] = [1546, 'u16'];
export const regen_battery: [number, string] = [1548, 'u16'];
export const energy_extracted_from_battery: [number, string] = [1550, 'u16'];
export const max_fastcharging_current_amp: [number, string] = [1552, 'u16'];
export const BMS_Status_DCLS: [number, string] = [1554, 'u8'];
export const DC_voltage_DCLS: [number, string] = [1556, 'u16'];
export const DC_voltage_chargeport: [number, string] = [1558, 'u16'];
export const BMS_welded_contactors_status: [number, string] = [1560, 'u8'];
export const BMS_error_shutdown_request: [number, string] = [1561, 'b'];
export const BMS_error_shutdown: [number, string] = [1562, 'b'];
export const power_battery_heating_watt: [number, string] = [1564, 'u16'];
export const power_battery_heating_req_watt: [number, string] = [1566, 'u16'];
export const cooling_request: [number, string] = [1568, 'u8'];
export const heating_request: [number, string] = [1569, 'u8'];
export const balancing_active: [number, string] = [1570, 'u8'];
export const charging_active: [number, string] = [1571, 'b'];
export const max_energy_Wh: [number, string] = [1572, 'u16'];
export const max_charge_percent: [number, string] = [1574, 'u16'];
export const min_charge_percent: [number, string] = [1576, 'u16'];
export const isolation_resistance_kOhm: [number, string] = [1578, 'u16'];
export const battery_heating_installed: [number, string] = [1580, 'b'];
export const error_NT_circuit: [number, string] = [1581, 'b'];
export const pump_1_control: [number, string] = [1582, 'u8'];
export const pump_2_control: [number, string] = [1583, 'u8'];
export const target_flow_temperature_C: [number, string] = [1584, 'u8'];
export const return_temperature_C: [number, string] = [1585, 'u8'];
export const status_valve_1: [number, string] = [1586, 'u8'];
export const status_valve_2: [number, string] = [1587, 'u8'];
export const temperature_request: [number, string] = [1588, 'u8'];
export const performance_index_discharge_peak_temperature_percentage: [number, string] = [1590, 'u16'];
export const performance_index_charge_peak_temperature_percentage: [number, string] = [1592, 'u16'];
export const temperature_status_discharge: [number, string] = [1594, 'u8'];
export const temperature_status_charge: [number, string] = [1595, 'u8'];
export const isolation_fault: [number, string] = [1596, 'u8'];
export const isolation_status: [number, string] = [1597, 'u8'];
export const actual_temperature_highest_C: [number, string] = [1598, 'u8'];
export const actual_temperature_lowest_C: [number, string] = [1599, 'u8'];
export const actual_cellvoltage_highest_mV: [number, string] = [1600, 'u16'];
export const actual_cellvoltage_lowest_mV: [number, string] = [1602, 'u16'];
export const predicted_power_dyn_standard_watt: [number, string] = [1604, 'u16'];
export const predicted_time_dyn_standard_minutes: [number, string] = [1606, 'u8'];
export const mux: [number, string] = [1607, 'u8'];
export const duration_discharge_power_watt: [number, string] = [1608, 'u16'];
export const duration_charge_power_watt: [number, string] = [1610, 'u16'];
export const maximum_voltage: [number, string] = [1612, 'u16'];
export const minimum_voltage: [number, string] = [1614, 'u16'];
export const battery_serialnumber: [number, string, number] = [1616, 'u8', 26];
export const realtime_overcurrent_monitor: [number, string] = [1642, 'u8'];
export const realtime_CAN_communication_fault: [number, string] = [1643, 'u8'];
export const realtime_overcharge_warning: [number, string] = [1644, 'u8'];
export const realtime_SOC_too_high: [number, string] = [1645, 'u8'];
export const realtime_SOC_too_low: [number, string] = [1646, 'u8'];
export const realtime_SOC_jumping_warning: [number, string] = [1647, 'u8'];
export const realtime_temperature_difference_warning: [number, string] = [1648, 'u8'];
export const realtime_cell_overtemperature_warning: [number, string] = [1649, 'u8'];
export const realtime_cell_undertemperature_warning: [number, string] = [1650, 'u8'];
export const realtime_battery_overvoltage_warning: [number, string] = [1651, 'u8'];
export const realtime_battery_undervoltage_warning: [number, string] = [1652, 'u8'];
export const realtime_cell_overvoltage_warning: [number, string] = [1653, 'u8'];
export const realtime_cell_undervoltage_warning: [number, string] = [1654, 'u8'];
export const realtime_cell_imbalance_warning: [number, string] = [1655, 'u8'];
export const realtime_warning_battery_unathorized: [number, string] = [1656, 'u8'];
export const component_protection_active: [number, string] = [1657, 'b'];
export const shutdown_active: [number, string] = [1658, 'b'];
export const transportation_mode_active: [number, string] = [1659, 'b'];
export const KL15_mode: [number, string] = [1660, 'u8'];
export const bus_knockout_timer: [number, string] = [1661, 'u8'];
export const hybrid_wakeup_reason: [number, string] = [1662, 'u8'];
export const wakeup_type: [number, string] = [1663, 'u8'];
export const instrument_cluster_request: [number, string] = [1664, 'b'];
export const seconds: [number, string] = [1665, 'u8'];
export const first_can_msg_timestamp: [number, string] = [1668, 'u32'];
export const last_can_msg_timestamp: [number, string] = [1672, 'u32'];
export const hv_requested: [number, string] = [1676, 'b'];
export const kwh_charge: [number, string] = [1680, 'i32'];
export const kwh_discharge: [number, string] = [1684, 'i32'];
export const Airbag_01_frame: [number, string] = [1688, 'CAN_frame'];
export const EM1_01_frame: [number, string] = [1760, 'CAN_frame'];
export const ESC_51_Auth_frame: [number, string] = [1832, 'CAN_frame'];
export const Diagnose_01_frame: [number, string] = [1904, 'CAN_frame'];
export const Temperaturen_01_frame: [number, string] = [1976, 'CAN_frame'];
export const Motor_Code_01_frame: [number, string] = [2048, 'CAN_frame'];
export const Klemmen_Status_01_frame: [number, string] = [2120, 'CAN_frame'];
export const ESP_21_frame: [number, string] = [2192, 'CAN_frame'];
export const NMH_Gateway_frame: [number, string] = [2264, 'CAN_frame'];
export const Motor_14_frame: [number, string] = [2336, 'CAN_frame'];
export const HVLM_14_frame: [number, string] = [2408, 'CAN_frame'];
export const HVK_01_frame: [number, string] = [2480, 'CAN_frame'];
export const Motor_54_frame: [number, string] = [2552, 'CAN_frame'];
export const Motor_EV_01_frame: [number, string] = [2624, 'CAN_frame'];
export const can_msg_received: [number, string] = [2696, 'u32'];
