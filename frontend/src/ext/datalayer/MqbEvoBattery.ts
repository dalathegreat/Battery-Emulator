// MqbEvoBattery: 2704 bytes; base classes: MebBattery@0
export const MQB_EVO_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['____vptr$Battery', '__vtbl_ptr_type*'],
  ['___defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['____vptr$Transmitter', '__vtbl_ptr_type*'],
  ['____vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['__can_interface', 'CAN_Interface'],
  ['__initial_speed', 'CAN_Speed'],
  ['__idx', 'i32'],
  ['__bs', 'u8'],
  ['___vptr$IsoTp', '__vtbl_ptr_type*'],
  ['__stmin', 'u8'],
  ['__len', 'i32'],
  ['___rx', 'TpCon'],
  ['__state', 'i32'],
  ['__bs_2', 'u8'],
  ['__sn', 'u8'],
  ['__buf', 'u8', 512],
  ['___tx', 'TpCon'],
  ['___rxfc', 'FcOpts'],
  ['___txfc', 'FcOpts'],
  ['___tx_wft', 'u8'],
  ['___tatype', 'isotp_tatype'],
  ['___rxtimer', 'u32'],
  ['___txtimer', 'u32'],
  ['___tx_id', 'u32'],
  ['___addrmode', 'isotp_addrmode'],
  ['___tx_addr', 'u8'],
  ['___rx_addr', 'u8'],
  ['_renderer', 'MebHtmlRenderer'],
  ['_datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['_datalayer_meb', 'DATALAYER_INFO_MEB*'],
  ['_platform', 'VAGPlatform'],
  ['_previousMillis10ms', 'u32'],
  ['_previousMillis20ms', 'u32'],
  ['_previousMillis40ms', 'u32'],
  ['_previousMillis50ms', 'u32'],
  ['_previousMillis100ms', 'u32'],
  ['_previousMillis200ms', 'u32'],
  ['_previousMillis500ms', 'u32'],
  ['_previousMillis1s', 'u32'],
  ['_toggle', 'b'],
  ['_counter_1000ms', 'u8'],
  ['_counter_200ms', 'u8'],
  ['_counter_100ms', 'u8'],
  ['_counter_50ms', 'u8'],
  ['_counter_40ms', 'u8'],
  ['_counter_20ms', 'u8'],
  ['_counter_10ms', 'u8'],
  ['_counter_040', 'u8'],
  ['_counter_0F7', 'u8'],
  ['_counter_3b5', 'u8'],
  ['_BMS_04_counter', 'u8'],
  ['_BMS_07_counter', 'u8'],
  ['_BMS_20_counter', 'u8'],
  ['_EM1_01_counter', 'u8'],
  ['_BMS_DC_01_counter', 'u8'],
  ['_BMS_11_counter', 'u8'],
  ['_BMS_11_CRC', 'u8'],
  ['_uds_request_pending', 'b'],
  ['_uds_request_timestamp', 'u32'],
  ['_basic_settings_state', 'BasicSettingsState'],
  ['_basic_settings_routine_id', 'u16'],
  ['_basic_settings_routine_param', 'u16'],
  ['_security_access_seed', 'u32'],
  ['_security_login_key', 'u32'],
  ['_basic_settings_wait_ms', 'u32'],
  ['_bms_reset_state', 'BmsResetState'],
  ['_bms_reset_ms', 'u32'],
  ['_bms_reset_active', 'b'],
  ['_bms_reset_tx_suppressed', 'b'],
  ['_startup_bms_checked', 'b'],
  ['_poll_pid', 'u32'],
  ['_nof_cells_determined', 'b'],
  ['_pid_reply', 'u32'],
  ['_battery_soc_polled', 'u16'],
  ['_battery_soh_polled', 'u16'],
  ['_battery_voltage_polled', 'u16'],
  ['_battery_current_polled', 'i16'],
  ['_battery_max_temp', 'i16'],
  ['_battery_min_temp', 'i16'],
  ['_battery_max_charge_voltage', 'u16'],
  ['_battery_min_discharge_voltage', 'u16'],
  ['_battery_allowed_charge_power', 'u16'],
  ['_battery_allowed_discharge_power', 'u16'],
  ['_cellvoltages_polled', 'u16', 108],
  ['_tempval', 'u16'],
  ['_BMS_ext_limits_active', 'b'],
  ['_BMS_mode', 'u8'],
  ['_BMS_HVIL_status', 'u8'],
  ['_BMS_fault_performance', 'b'],
  ['_BMS_current', 'u16'],
  ['_BMS_fault_emergency_shutdown_crash', 'b'],
  ['_BMS_voltage_intermediate', 'u32'],
  ['_BMS_voltage', 'u32'],
  ['_BMS_usable_batt_energy_Wh', 'i32'],
  ['_BMS_usable_batt_energy_t_Wh', 'i32'],
  ['_BMS_max_usable_batt_energy_Wh', 'i32'],
  ['_BMS_nominal_voltage_dV', 'u16'],
  ['_BMS_status_voltage_free', 'u8'],
  ['_BMS_OBD_MIL', 'b'],
  ['_BMS_error_status', 'u8'],
  ['_BMS_capacity_ah', 'u16'],
  ['_BMS_error_lamp_req', 'b'],
  ['_BMS_warning_lamp_req', 'b'],
  ['_BMS_Kl30c_Status', 'u8'],
  ['_service_disconnect_switch_missing', 'b'],
  ['_pilotline_open', 'b'],
  ['_balancing_request', 'b'],
  ['_battery_diagnostic', 'u8'],
  ['_battery_Wh_left', 'u16'],
  ['_battery_Wh_max', 'u16'],
  ['_battery_potential_status', 'u8'],
  ['_battery_temperature_warning', 'u8'],
  ['_max_discharge_power_watt', 'u16'],
  ['_max_discharge_current_amp', 'u16'],
  ['_max_charge_power_watt', 'u16'],
  ['_max_charge_current_amp', 'u16'],
  ['_battery_SOC', 'u16'],
  ['_usable_energy_amount_Wh', 'u16'],
  ['_status_HV_PTC_line', 'u8'],
  ['_warning_support', 'u8'],
  ['_battery_heating_active', 'b'],
  ['_power_discharge_percentage', 'u16'],
  ['_power_charge_percentage', 'u16'],
  ['_actual_battery_voltage', 'u16'],
  ['_regen_battery', 'u16'],
  ['_energy_extracted_from_battery', 'u16'],
  ['_max_fastcharging_current_amp', 'u16'],
  ['_BMS_Status_DCLS', 'u8'],
  ['_DC_voltage_DCLS', 'u16'],
  ['_DC_voltage_chargeport', 'u16'],
  ['_BMS_welded_contactors_status', 'u8'],
  ['_BMS_error_shutdown_request', 'b'],
  ['_BMS_error_shutdown', 'b'],
  ['_power_battery_heating_watt', 'u16'],
  ['_power_battery_heating_req_watt', 'u16'],
  ['_cooling_request', 'u8'],
  ['_heating_request', 'u8'],
  ['_balancing_active', 'u8'],
  ['_charging_active', 'b'],
  ['_max_energy_Wh', 'u16'],
  ['_max_charge_percent', 'u16'],
  ['_min_charge_percent', 'u16'],
  ['_isolation_resistance_kOhm', 'u16'],
  ['_battery_heating_installed', 'b'],
  ['_error_NT_circuit', 'b'],
  ['_pump_1_control', 'u8'],
  ['_pump_2_control', 'u8'],
  ['_target_flow_temperature_C', 'u8'],
  ['_return_temperature_C', 'u8'],
  ['_status_valve_1', 'u8'],
  ['_status_valve_2', 'u8'],
  ['_temperature_request', 'u8'],
  ['_performance_index_discharge_peak_temperature_percentage', 'u16'],
  ['_performance_index_charge_peak_temperature_percentage', 'u16'],
  ['_temperature_status_discharge', 'u8'],
  ['_temperature_status_charge', 'u8'],
  ['_isolation_fault', 'u8'],
  ['_isolation_status', 'u8'],
  ['_actual_temperature_highest_C', 'u8'],
  ['_actual_temperature_lowest_C', 'u8'],
  ['_actual_cellvoltage_highest_mV', 'u16'],
  ['_actual_cellvoltage_lowest_mV', 'u16'],
  ['_predicted_power_dyn_standard_watt', 'u16'],
  ['_predicted_time_dyn_standard_minutes', 'u8'],
  ['_mux', 'u8'],
  ['_duration_discharge_power_watt', 'u16'],
  ['_duration_charge_power_watt', 'u16'],
  ['_maximum_voltage', 'u16'],
  ['_minimum_voltage', 'u16'],
  ['_battery_serialnumber', 'u8', 26],
  ['_realtime_overcurrent_monitor', 'u8'],
  ['_realtime_CAN_communication_fault', 'u8'],
  ['_realtime_overcharge_warning', 'u8'],
  ['_realtime_SOC_too_high', 'u8'],
  ['_realtime_SOC_too_low', 'u8'],
  ['_realtime_SOC_jumping_warning', 'u8'],
  ['_realtime_temperature_difference_warning', 'u8'],
  ['_realtime_cell_overtemperature_warning', 'u8'],
  ['_realtime_cell_undertemperature_warning', 'u8'],
  ['_realtime_battery_overvoltage_warning', 'u8'],
  ['_realtime_battery_undervoltage_warning', 'u8'],
  ['_realtime_cell_overvoltage_warning', 'u8'],
  ['_realtime_cell_undervoltage_warning', 'u8'],
  ['_realtime_cell_imbalance_warning', 'u8'],
  ['_realtime_warning_battery_unathorized', 'u8'],
  ['_component_protection_active', 'b'],
  ['_shutdown_active', 'b'],
  ['_transportation_mode_active', 'b'],
  ['_KL15_mode', 'u8'],
  ['_bus_knockout_timer', 'u8'],
  ['_hybrid_wakeup_reason', 'u8'],
  ['_wakeup_type', 'u8'],
  ['_instrument_cluster_request', 'b'],
  ['_seconds', 'u8'],
  ['_first_can_msg_timestamp', 'u32'],
  ['_last_can_msg_timestamp', 'u32'],
  ['_hv_requested', 'b'],
  ['_kwh_charge', 'i32'],
  ['_kwh_discharge', 'i32'],
  ['_Airbag_01_frame', 'CAN_frame'],
  ['_EM1_01_frame', 'CAN_frame'],
  ['_ESC_51_Auth_frame', 'CAN_frame'],
  ['_Diagnose_01_frame', 'CAN_frame'],
  ['_Temperaturen_01_frame', 'CAN_frame'],
  ['_Motor_Code_01_frame', 'CAN_frame'],
  ['_Klemmen_Status_01_frame', 'CAN_frame'],
  ['_ESP_21_frame', 'CAN_frame'],
  ['_NMH_Gateway_frame', 'CAN_frame'],
  ['_Motor_14_frame', 'CAN_frame'],
  ['_HVLM_14_frame', 'CAN_frame'],
  ['_HVK_01_frame', 'CAN_frame'],
  ['_Motor_54_frame', 'CAN_frame'],
  ['_Motor_EV_01_frame', 'CAN_frame'],
  ['_can_msg_received', 'u32'],
];

export const ____vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const ___defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ____vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ____vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const __can_interface: [number, string] = [16, 'CAN_Interface'];
export const __initial_speed: [number, string] = [20, 'CAN_Speed'];
export const __idx: [number, string] = [24, 'i32'];
export const __bs: [number, string] = [24, 'u8'];
export const ___vptr$IsoTp: [number, string] = [24, '__vtbl_ptr_type*'];
export const __stmin: [number, string] = [25, 'u8'];
export const __len: [number, string] = [28, 'i32'];
export const ___rx: [number, string] = [28, 'TpCon'];
export const __state: [number, string] = [32, 'i32'];
export const __bs_2: [number, string] = [36, 'u8'];
export const __sn: [number, string] = [37, 'u8'];
export const __buf: [number, string, number] = [38, 'u8', 512];
export const ___tx: [number, string] = [556, 'TpCon'];
export const ___rxfc: [number, string] = [1084, 'FcOpts'];
export const ___txfc: [number, string] = [1086, 'FcOpts'];
export const ___tx_wft: [number, string] = [1088, 'u8'];
export const ___tatype: [number, string] = [1092, 'isotp_tatype'];
export const ___rxtimer: [number, string] = [1096, 'u32'];
export const ___txtimer: [number, string] = [1100, 'u32'];
export const ___tx_id: [number, string] = [1104, 'u32'];
export const ___addrmode: [number, string] = [1108, 'isotp_addrmode'];
export const ___tx_addr: [number, string] = [1112, 'u8'];
export const ___rx_addr: [number, string] = [1113, 'u8'];
export const _renderer: [number, string] = [1116, 'MebHtmlRenderer'];
export const _datalayer_battery: [number, string] = [1124, 'DATALAYER_BATTERY_TYPE*'];
export const _datalayer_meb: [number, string] = [1128, 'DATALAYER_INFO_MEB*'];
export const _platform: [number, string] = [1132, 'VAGPlatform'];
export const _previousMillis10ms: [number, string] = [1136, 'u32'];
export const _previousMillis20ms: [number, string] = [1140, 'u32'];
export const _previousMillis40ms: [number, string] = [1144, 'u32'];
export const _previousMillis50ms: [number, string] = [1148, 'u32'];
export const _previousMillis100ms: [number, string] = [1152, 'u32'];
export const _previousMillis200ms: [number, string] = [1156, 'u32'];
export const _previousMillis500ms: [number, string] = [1160, 'u32'];
export const _previousMillis1s: [number, string] = [1164, 'u32'];
export const _toggle: [number, string] = [1168, 'b'];
export const _counter_1000ms: [number, string] = [1169, 'u8'];
export const _counter_200ms: [number, string] = [1170, 'u8'];
export const _counter_100ms: [number, string] = [1171, 'u8'];
export const _counter_50ms: [number, string] = [1172, 'u8'];
export const _counter_40ms: [number, string] = [1173, 'u8'];
export const _counter_20ms: [number, string] = [1174, 'u8'];
export const _counter_10ms: [number, string] = [1175, 'u8'];
export const _counter_040: [number, string] = [1176, 'u8'];
export const _counter_0F7: [number, string] = [1177, 'u8'];
export const _counter_3b5: [number, string] = [1178, 'u8'];
export const _BMS_04_counter: [number, string] = [1179, 'u8'];
export const _BMS_07_counter: [number, string] = [1180, 'u8'];
export const _BMS_20_counter: [number, string] = [1181, 'u8'];
export const _EM1_01_counter: [number, string] = [1182, 'u8'];
export const _BMS_DC_01_counter: [number, string] = [1183, 'u8'];
export const _BMS_11_counter: [number, string] = [1184, 'u8'];
export const _BMS_11_CRC: [number, string] = [1185, 'u8'];
export const _uds_request_pending: [number, string] = [1186, 'b'];
export const _uds_request_timestamp: [number, string] = [1188, 'u32'];
export const _basic_settings_state: [number, string] = [1192, 'BasicSettingsState'];
export const _basic_settings_routine_id: [number, string] = [1194, 'u16'];
export const _basic_settings_routine_param: [number, string] = [1196, 'u16'];
export const _security_access_seed: [number, string] = [1200, 'u32'];
export const _security_login_key: [number, string] = [1204, 'u32'];
export const _basic_settings_wait_ms: [number, string] = [1208, 'u32'];
export const _bms_reset_state: [number, string] = [1212, 'BmsResetState'];
export const _bms_reset_ms: [number, string] = [1216, 'u32'];
export const _bms_reset_active: [number, string] = [1220, 'b'];
export const _bms_reset_tx_suppressed: [number, string] = [1221, 'b'];
export const _startup_bms_checked: [number, string] = [1222, 'b'];
export const _poll_pid: [number, string] = [1224, 'u32'];
export const _nof_cells_determined: [number, string] = [1228, 'b'];
export const _pid_reply: [number, string] = [1232, 'u32'];
export const _battery_soc_polled: [number, string] = [1236, 'u16'];
export const _battery_soh_polled: [number, string] = [1238, 'u16'];
export const _battery_voltage_polled: [number, string] = [1240, 'u16'];
export const _battery_current_polled: [number, string] = [1242, 'i16'];
export const _battery_max_temp: [number, string] = [1244, 'i16'];
export const _battery_min_temp: [number, string] = [1246, 'i16'];
export const _battery_max_charge_voltage: [number, string] = [1248, 'u16'];
export const _battery_min_discharge_voltage: [number, string] = [1250, 'u16'];
export const _battery_allowed_charge_power: [number, string] = [1252, 'u16'];
export const _battery_allowed_discharge_power: [number, string] = [1254, 'u16'];
export const _cellvoltages_polled: [number, string, number] = [1256, 'u16', 108];
export const _tempval: [number, string] = [1472, 'u16'];
export const _BMS_ext_limits_active: [number, string] = [1474, 'b'];
export const _BMS_mode: [number, string] = [1475, 'u8'];
export const _BMS_HVIL_status: [number, string] = [1476, 'u8'];
export const _BMS_fault_performance: [number, string] = [1477, 'b'];
export const _BMS_current: [number, string] = [1478, 'u16'];
export const _BMS_fault_emergency_shutdown_crash: [number, string] = [1480, 'b'];
export const _BMS_voltage_intermediate: [number, string] = [1484, 'u32'];
export const _BMS_voltage: [number, string] = [1488, 'u32'];
export const _BMS_usable_batt_energy_Wh: [number, string] = [1492, 'i32'];
export const _BMS_usable_batt_energy_t_Wh: [number, string] = [1496, 'i32'];
export const _BMS_max_usable_batt_energy_Wh: [number, string] = [1500, 'i32'];
export const _BMS_nominal_voltage_dV: [number, string] = [1504, 'u16'];
export const _BMS_status_voltage_free: [number, string] = [1506, 'u8'];
export const _BMS_OBD_MIL: [number, string] = [1507, 'b'];
export const _BMS_error_status: [number, string] = [1508, 'u8'];
export const _BMS_capacity_ah: [number, string] = [1510, 'u16'];
export const _BMS_error_lamp_req: [number, string] = [1512, 'b'];
export const _BMS_warning_lamp_req: [number, string] = [1513, 'b'];
export const _BMS_Kl30c_Status: [number, string] = [1514, 'u8'];
export const _service_disconnect_switch_missing: [number, string] = [1515, 'b'];
export const _pilotline_open: [number, string] = [1516, 'b'];
export const _balancing_request: [number, string] = [1517, 'b'];
export const _battery_diagnostic: [number, string] = [1518, 'u8'];
export const _battery_Wh_left: [number, string] = [1520, 'u16'];
export const _battery_Wh_max: [number, string] = [1522, 'u16'];
export const _battery_potential_status: [number, string] = [1524, 'u8'];
export const _battery_temperature_warning: [number, string] = [1525, 'u8'];
export const _max_discharge_power_watt: [number, string] = [1526, 'u16'];
export const _max_discharge_current_amp: [number, string] = [1528, 'u16'];
export const _max_charge_power_watt: [number, string] = [1530, 'u16'];
export const _max_charge_current_amp: [number, string] = [1532, 'u16'];
export const _battery_SOC: [number, string] = [1534, 'u16'];
export const _usable_energy_amount_Wh: [number, string] = [1536, 'u16'];
export const _status_HV_PTC_line: [number, string] = [1538, 'u8'];
export const _warning_support: [number, string] = [1539, 'u8'];
export const _battery_heating_active: [number, string] = [1540, 'b'];
export const _power_discharge_percentage: [number, string] = [1542, 'u16'];
export const _power_charge_percentage: [number, string] = [1544, 'u16'];
export const _actual_battery_voltage: [number, string] = [1546, 'u16'];
export const _regen_battery: [number, string] = [1548, 'u16'];
export const _energy_extracted_from_battery: [number, string] = [1550, 'u16'];
export const _max_fastcharging_current_amp: [number, string] = [1552, 'u16'];
export const _BMS_Status_DCLS: [number, string] = [1554, 'u8'];
export const _DC_voltage_DCLS: [number, string] = [1556, 'u16'];
export const _DC_voltage_chargeport: [number, string] = [1558, 'u16'];
export const _BMS_welded_contactors_status: [number, string] = [1560, 'u8'];
export const _BMS_error_shutdown_request: [number, string] = [1561, 'b'];
export const _BMS_error_shutdown: [number, string] = [1562, 'b'];
export const _power_battery_heating_watt: [number, string] = [1564, 'u16'];
export const _power_battery_heating_req_watt: [number, string] = [1566, 'u16'];
export const _cooling_request: [number, string] = [1568, 'u8'];
export const _heating_request: [number, string] = [1569, 'u8'];
export const _balancing_active: [number, string] = [1570, 'u8'];
export const _charging_active: [number, string] = [1571, 'b'];
export const _max_energy_Wh: [number, string] = [1572, 'u16'];
export const _max_charge_percent: [number, string] = [1574, 'u16'];
export const _min_charge_percent: [number, string] = [1576, 'u16'];
export const _isolation_resistance_kOhm: [number, string] = [1578, 'u16'];
export const _battery_heating_installed: [number, string] = [1580, 'b'];
export const _error_NT_circuit: [number, string] = [1581, 'b'];
export const _pump_1_control: [number, string] = [1582, 'u8'];
export const _pump_2_control: [number, string] = [1583, 'u8'];
export const _target_flow_temperature_C: [number, string] = [1584, 'u8'];
export const _return_temperature_C: [number, string] = [1585, 'u8'];
export const _status_valve_1: [number, string] = [1586, 'u8'];
export const _status_valve_2: [number, string] = [1587, 'u8'];
export const _temperature_request: [number, string] = [1588, 'u8'];
export const _performance_index_discharge_peak_temperature_percentage: [number, string] = [1590, 'u16'];
export const _performance_index_charge_peak_temperature_percentage: [number, string] = [1592, 'u16'];
export const _temperature_status_discharge: [number, string] = [1594, 'u8'];
export const _temperature_status_charge: [number, string] = [1595, 'u8'];
export const _isolation_fault: [number, string] = [1596, 'u8'];
export const _isolation_status: [number, string] = [1597, 'u8'];
export const _actual_temperature_highest_C: [number, string] = [1598, 'u8'];
export const _actual_temperature_lowest_C: [number, string] = [1599, 'u8'];
export const _actual_cellvoltage_highest_mV: [number, string] = [1600, 'u16'];
export const _actual_cellvoltage_lowest_mV: [number, string] = [1602, 'u16'];
export const _predicted_power_dyn_standard_watt: [number, string] = [1604, 'u16'];
export const _predicted_time_dyn_standard_minutes: [number, string] = [1606, 'u8'];
export const _mux: [number, string] = [1607, 'u8'];
export const _duration_discharge_power_watt: [number, string] = [1608, 'u16'];
export const _duration_charge_power_watt: [number, string] = [1610, 'u16'];
export const _maximum_voltage: [number, string] = [1612, 'u16'];
export const _minimum_voltage: [number, string] = [1614, 'u16'];
export const _battery_serialnumber: [number, string, number] = [1616, 'u8', 26];
export const _realtime_overcurrent_monitor: [number, string] = [1642, 'u8'];
export const _realtime_CAN_communication_fault: [number, string] = [1643, 'u8'];
export const _realtime_overcharge_warning: [number, string] = [1644, 'u8'];
export const _realtime_SOC_too_high: [number, string] = [1645, 'u8'];
export const _realtime_SOC_too_low: [number, string] = [1646, 'u8'];
export const _realtime_SOC_jumping_warning: [number, string] = [1647, 'u8'];
export const _realtime_temperature_difference_warning: [number, string] = [1648, 'u8'];
export const _realtime_cell_overtemperature_warning: [number, string] = [1649, 'u8'];
export const _realtime_cell_undertemperature_warning: [number, string] = [1650, 'u8'];
export const _realtime_battery_overvoltage_warning: [number, string] = [1651, 'u8'];
export const _realtime_battery_undervoltage_warning: [number, string] = [1652, 'u8'];
export const _realtime_cell_overvoltage_warning: [number, string] = [1653, 'u8'];
export const _realtime_cell_undervoltage_warning: [number, string] = [1654, 'u8'];
export const _realtime_cell_imbalance_warning: [number, string] = [1655, 'u8'];
export const _realtime_warning_battery_unathorized: [number, string] = [1656, 'u8'];
export const _component_protection_active: [number, string] = [1657, 'b'];
export const _shutdown_active: [number, string] = [1658, 'b'];
export const _transportation_mode_active: [number, string] = [1659, 'b'];
export const _KL15_mode: [number, string] = [1660, 'u8'];
export const _bus_knockout_timer: [number, string] = [1661, 'u8'];
export const _hybrid_wakeup_reason: [number, string] = [1662, 'u8'];
export const _wakeup_type: [number, string] = [1663, 'u8'];
export const _instrument_cluster_request: [number, string] = [1664, 'b'];
export const _seconds: [number, string] = [1665, 'u8'];
export const _first_can_msg_timestamp: [number, string] = [1668, 'u32'];
export const _last_can_msg_timestamp: [number, string] = [1672, 'u32'];
export const _hv_requested: [number, string] = [1676, 'b'];
export const _kwh_charge: [number, string] = [1680, 'i32'];
export const _kwh_discharge: [number, string] = [1684, 'i32'];
export const _Airbag_01_frame: [number, string] = [1688, 'CAN_frame'];
export const _EM1_01_frame: [number, string] = [1760, 'CAN_frame'];
export const _ESC_51_Auth_frame: [number, string] = [1832, 'CAN_frame'];
export const _Diagnose_01_frame: [number, string] = [1904, 'CAN_frame'];
export const _Temperaturen_01_frame: [number, string] = [1976, 'CAN_frame'];
export const _Motor_Code_01_frame: [number, string] = [2048, 'CAN_frame'];
export const _Klemmen_Status_01_frame: [number, string] = [2120, 'CAN_frame'];
export const _ESP_21_frame: [number, string] = [2192, 'CAN_frame'];
export const _NMH_Gateway_frame: [number, string] = [2264, 'CAN_frame'];
export const _Motor_14_frame: [number, string] = [2336, 'CAN_frame'];
export const _HVLM_14_frame: [number, string] = [2408, 'CAN_frame'];
export const _HVK_01_frame: [number, string] = [2480, 'CAN_frame'];
export const _Motor_54_frame: [number, string] = [2552, 'CAN_frame'];
export const _Motor_EV_01_frame: [number, string] = [2624, 'CAN_frame'];
export const _can_msg_received: [number, string] = [2696, 'u32'];
