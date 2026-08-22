// BmwI3Battery: 1648 bytes; base classes: CanBattery@0
export const BMW_I3_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['renderer', 'BmwI3HtmlRenderer'],
  ['', ' ', 8],
  ['UserRequestDTCreset', 'b'],
  ['', ' ', 3],
  ['UserRequestBalancing', 'BalancingState'],
  ['UserRequestBalancingMillis', 'u32'],
  ['MAX_CELL_VOLTAGE_60AH', 'i32'],
  ['MIN_CELL_VOLTAGE_60AH', 'i32'],
  ['MAX_CELL_VOLTAGE_94AH', 'i32'],
  ['MIN_CELL_VOLTAGE_94AH', 'i32'],
  ['MAX_CELL_VOLTAGE_120AH', 'i32'],
  ['MIN_CELL_VOLTAGE_120AH', 'i32'],
  ['MAX_CELL_DEVIATION_MV', 'i32'],
  ['MAX_PACK_VOLTAGE_60AH', 'i32'],
  ['MIN_PACK_VOLTAGE_60AH', 'i32'],
  ['MAX_PACK_VOLTAGE_94AH', 'i32'],
  ['MIN_PACK_VOLTAGE_94AH', 'i32'],
  ['MAX_PACK_VOLTAGE_120AH', 'i32'],
  ['MIN_PACK_VOLTAGE_120AH', 'i32'],
  ['NUMBER_OF_CELLS', 'i32'],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['allows_contactor_closing', 'bool*'],
  ['contactor_closing_allowed', 'bool*'],
  ['wakeup_pin', 'gpio_num_t'],
  ['previousMillis20', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis200', 'u32'],
  ['previousMillis500', 'u32'],
  ['previousMillis640', 'u32'],
  ['previousMillis1000', 'u32'],
  ['previousMillis5000', 'u32'],
  ['previousMillis10000', 'u32'],
  ['detectedBattery', 'BatterySize'],
  ['cmdState', 'CmdState'],
  ['', ' ', 4],
  ['BMW_10B', 'CAN_frame'],
  ['BMW_12F', 'CAN_frame'],
  ['BMW_13E', 'CAN_frame'],
  ['BMW_19B', 'CAN_frame'],
  ['BMW_1D0', 'CAN_frame'],
  ['BMW_30B', 'CAN_frame'],
  ['BMW_328', 'CAN_frame'],
  ['BMW_3A7', 'CAN_frame'],
  ['BMW_3C5', 'CAN_frame'],
  ['BMW_3E5', 'CAN_frame'],
  ['BMW_3E8', 'CAN_frame'],
  ['BMW_3E9', 'CAN_frame'],
  ['BMW_3EC', 'CAN_frame'],
  ['BMW_3F9', 'CAN_frame'],
  ['BMW_3FC', 'CAN_frame'],
  ['BMW_433', 'CAN_frame'],
  ['BMW_6F4_CELL_VOLTAGE_CELLNO', 'CAN_frame'],
  ['balancing_12F_times', '12d2b'],
  ['', ' ', 36],
  ['balancing_12F_values', '12d40'],
  ['', ' ', 9],
  ['balancing_mode_active', 'b'],
  ['can_communication_stopped', 'b'],
  ['', ' '],
  ['balancing_start_time', 'u32'],
  ['startup_counter_contactor', 'u8'],
  ['alive_counter_20ms', 'u8'],
  ['alive_counter_100ms', 'u8'],
  ['alive_counter_200ms', 'u8'],
  ['alive_counter_500ms', 'u8'],
  ['alive_counter_1000ms', 'u8'],
  ['alive_counter_5000ms', 'u8'],
  ['BMW_1D0_counter', 'u8'],
  ['BMW_13E_counter', 'u8'],
  ['BMW_380_counter', 'u8'],
  ['', ' ', 2],
  ['BMW_328_seconds', 'u32'],
  ['BMW_328_days', 'u16'],
  ['', ' ', 2],
  ['BMS_328_seconds_to_day', 'u32'],
  ['battery_awake', 'b'],
  ['battery_info_available', 'b'],
  ['skipCRCCheck', 'b'],
  ['CRCCheckPassedPreviously', 'b'],
  ['cellvoltage_temp_mV', 'u16'],
  ['', ' ', 2],
  ['battery_serial_number', 'u32'],
  ['battery_available_power_shortterm_charge', 'u32'],
  ['battery_available_power_shortterm_discharge', 'u32'],
  ['battery_available_power_longterm_charge', 'u32'],
  ['battery_available_power_longterm_discharge', 'u32'],
  ['battery_BEV_available_power_shortterm_charge', 'u32'],
  ['battery_BEV_available_power_shortterm_discharge', 'u32'],
  ['battery_BEV_available_power_longterm_charge', 'u32'],
  ['battery_BEV_available_power_longterm_discharge', 'u32'],
  ['battery_energy_content_maximum_Wh', 'u16'],
  ['battery_display_SOC', 'u16'],
  ['battery_volts', 'u16'],
  ['temp_voltage', 'u16'],
  ['battery_HVBatt_SOC', 'u16'],
  ['battery_DC_link_voltage', 'u16'],
  ['battery_max_charge_voltage', 'u16'],
  ['battery_min_discharge_voltage', 'u16'],
  ['battery_predicted_energy_charge_condition', 'u16'],
  ['battery_predicted_energy_charging_target', 'u16'],
  ['battery_actual_value_power_heating', 'u16'],
  ['battery_prediction_voltage_shortterm_charge', 'u16'],
  ['battery_prediction_voltage_shortterm_discharge', 'u16'],
  ['battery_prediction_voltage_longterm_charge', 'u16'],
  ['battery_prediction_voltage_longterm_discharge', 'u16'],
  ['battery_prediction_duration_charging_minutes', 'u16'],
  ['battery_target_voltage_in_CV_mode', 'u16'],
  ['battery_soc', 'u16'],
  ['battery_soc_hvmax', 'u16'],
  ['battery_soc_hvmin', 'u16'],
  ['battery_capacity_cah', 'u16'],
  ['battery_temperature_HV', 'i16'],
  ['battery_temperature_heat_exchanger', 'i16'],
  ['battery_temperature_max', 'i16'],
  ['battery_temperature_min', 'i16'],
  ['battery_max_charge_amperage', 'i16'],
  ['battery_max_discharge_amperage', 'i16'],
  ['battery_current', 'i16'],
  ['battery_status_error_isolation_external_Bordnetz', 'u8'],
  ['battery_status_error_isolation_internal_Bordnetz', 'u8'],
  ['battery_request_cooling', 'u8'],
  ['battery_status_valve_cooling', 'u8'],
  ['battery_status_error_locking', 'u8'],
  ['battery_status_precharge_locked', 'u8'],
  ['battery_status_disconnecting_switch', 'u8'],
  ['battery_status_emergency_mode', 'u8'],
  ['battery_request_service', 'u8'],
  ['battery_error_emergency_mode', 'u8'],
  ['battery_status_error_disconnecting_switch', 'u8'],
  ['battery_status_warning_isolation', 'u8'],
  ['battery_status_cold_shutoff_valve', 'u8'],
  ['battery_request_open_contactors', 'u8'],
  ['battery_request_open_contactors_instantly', 'u8'],
  ['battery_request_open_contactors_fast', 'u8'],
  ['battery_charging_condition_delta', 'u8'],
  ['battery_status_service_disconnection_plug', 'u8'],
  ['battery_status_measurement_isolation', 'u8'],
  ['battery_request_abort_charging', 'u8'],
  ['battery_prediction_time_end_of_charging_minutes', 'u8'],
  ['battery_request_operating_mode', 'u8'],
  ['battery_request_charging_condition_minimum', 'u8'],
  ['battery_request_charging_condition_maximum', 'u8'],
  ['battery_status_cooling_HV', 'u8'],
  ['battery_status_diagnostics_HV', 'u8'],
  ['battery_status_diagnosis_powertrain_maximum_multiplexer', 'u8'],
  ['battery_status_diagnosis_powertrain_immediate_multiplexer', 'u8'],
  ['battery_ID2', 'u8'],
  ['battery_soh', 'u8'],
  ['message_data', 'u8', 50],
  ['next_data', 'u8'],
  ['current_cell_polled', 'u8'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const renderer: [number, string] = [24, 'BmwI3HtmlRenderer'];
export const UserRequestDTCreset: [number, string] = [32, 'b'];
export const UserRequestBalancing: [number, string] = [36, 'BalancingState'];
export const UserRequestBalancingMillis: [number, string] = [40, 'u32'];
export const MAX_CELL_VOLTAGE_60AH: [number, string] = [44, 'i32'];
export const MIN_CELL_VOLTAGE_60AH: [number, string] = [48, 'i32'];
export const MAX_CELL_VOLTAGE_94AH: [number, string] = [52, 'i32'];
export const MIN_CELL_VOLTAGE_94AH: [number, string] = [56, 'i32'];
export const MAX_CELL_VOLTAGE_120AH: [number, string] = [60, 'i32'];
export const MIN_CELL_VOLTAGE_120AH: [number, string] = [64, 'i32'];
export const MAX_CELL_DEVIATION_MV: [number, string] = [68, 'i32'];
export const MAX_PACK_VOLTAGE_60AH: [number, string] = [72, 'i32'];
export const MIN_PACK_VOLTAGE_60AH: [number, string] = [76, 'i32'];
export const MAX_PACK_VOLTAGE_94AH: [number, string] = [80, 'i32'];
export const MIN_PACK_VOLTAGE_94AH: [number, string] = [84, 'i32'];
export const MAX_PACK_VOLTAGE_120AH: [number, string] = [88, 'i32'];
export const MIN_PACK_VOLTAGE_120AH: [number, string] = [92, 'i32'];
export const NUMBER_OF_CELLS: [number, string] = [96, 'i32'];
export const datalayer_battery: [number, string] = [100, 'DATALAYER_BATTERY_TYPE*'];
export const allows_contactor_closing: [number, string] = [104, 'bool*'];
export const contactor_closing_allowed: [number, string] = [108, 'bool*'];
export const wakeup_pin: [number, string] = [112, 'gpio_num_t'];
export const previousMillis20: [number, string] = [116, 'u32'];
export const previousMillis100: [number, string] = [120, 'u32'];
export const previousMillis200: [number, string] = [124, 'u32'];
export const previousMillis500: [number, string] = [128, 'u32'];
export const previousMillis640: [number, string] = [132, 'u32'];
export const previousMillis1000: [number, string] = [136, 'u32'];
export const previousMillis5000: [number, string] = [140, 'u32'];
export const previousMillis10000: [number, string] = [144, 'u32'];
export const detectedBattery: [number, string] = [148, 'BatterySize'];
export const cmdState: [number, string] = [152, 'CmdState'];
export const BMW_10B: [number, string] = [160, 'CAN_frame'];
export const BMW_12F: [number, string] = [232, 'CAN_frame'];
export const BMW_13E: [number, string] = [304, 'CAN_frame'];
export const BMW_19B: [number, string] = [376, 'CAN_frame'];
export const BMW_1D0: [number, string] = [448, 'CAN_frame'];
export const BMW_30B: [number, string] = [520, 'CAN_frame'];
export const BMW_328: [number, string] = [592, 'CAN_frame'];
export const BMW_3A7: [number, string] = [664, 'CAN_frame'];
export const BMW_3C5: [number, string] = [736, 'CAN_frame'];
export const BMW_3E5: [number, string] = [808, 'CAN_frame'];
export const BMW_3E8: [number, string] = [880, 'CAN_frame'];
export const BMW_3E9: [number, string] = [952, 'CAN_frame'];
export const BMW_3EC: [number, string] = [1024, 'CAN_frame'];
export const BMW_3F9: [number, string] = [1096, 'CAN_frame'];
export const BMW_3FC: [number, string] = [1168, 'CAN_frame'];
export const BMW_433: [number, string] = [1240, 'CAN_frame'];
export const BMW_6F4_CELL_VOLTAGE_CELLNO: [number, string] = [1312, 'CAN_frame'];
export const balancing_12F_times: [number, string] = [1384, '12d2b'];
export const balancing_12F_values: [number, string] = [1420, '12d40'];
export const balancing_mode_active: [number, string] = [1429, 'b'];
export const can_communication_stopped: [number, string] = [1430, 'b'];
export const balancing_start_time: [number, string] = [1432, 'u32'];
export const startup_counter_contactor: [number, string] = [1436, 'u8'];
export const alive_counter_20ms: [number, string] = [1437, 'u8'];
export const alive_counter_100ms: [number, string] = [1438, 'u8'];
export const alive_counter_200ms: [number, string] = [1439, 'u8'];
export const alive_counter_500ms: [number, string] = [1440, 'u8'];
export const alive_counter_1000ms: [number, string] = [1441, 'u8'];
export const alive_counter_5000ms: [number, string] = [1442, 'u8'];
export const BMW_1D0_counter: [number, string] = [1443, 'u8'];
export const BMW_13E_counter: [number, string] = [1444, 'u8'];
export const BMW_380_counter: [number, string] = [1445, 'u8'];
export const BMW_328_seconds: [number, string] = [1448, 'u32'];
export const BMW_328_days: [number, string] = [1452, 'u16'];
export const BMS_328_seconds_to_day: [number, string] = [1456, 'u32'];
export const battery_awake: [number, string] = [1460, 'b'];
export const battery_info_available: [number, string] = [1461, 'b'];
export const skipCRCCheck: [number, string] = [1462, 'b'];
export const CRCCheckPassedPreviously: [number, string] = [1463, 'b'];
export const cellvoltage_temp_mV: [number, string] = [1464, 'u16'];
export const battery_serial_number: [number, string] = [1468, 'u32'];
export const battery_available_power_shortterm_charge: [number, string] = [1472, 'u32'];
export const battery_available_power_shortterm_discharge: [number, string] = [1476, 'u32'];
export const battery_available_power_longterm_charge: [number, string] = [1480, 'u32'];
export const battery_available_power_longterm_discharge: [number, string] = [1484, 'u32'];
export const battery_BEV_available_power_shortterm_charge: [number, string] = [1488, 'u32'];
export const battery_BEV_available_power_shortterm_discharge: [number, string] = [1492, 'u32'];
export const battery_BEV_available_power_longterm_charge: [number, string] = [1496, 'u32'];
export const battery_BEV_available_power_longterm_discharge: [number, string] = [1500, 'u32'];
export const battery_energy_content_maximum_Wh: [number, string] = [1504, 'u16'];
export const battery_display_SOC: [number, string] = [1506, 'u16'];
export const battery_volts: [number, string] = [1508, 'u16'];
export const temp_voltage: [number, string] = [1510, 'u16'];
export const battery_HVBatt_SOC: [number, string] = [1512, 'u16'];
export const battery_DC_link_voltage: [number, string] = [1514, 'u16'];
export const battery_max_charge_voltage: [number, string] = [1516, 'u16'];
export const battery_min_discharge_voltage: [number, string] = [1518, 'u16'];
export const battery_predicted_energy_charge_condition: [number, string] = [1520, 'u16'];
export const battery_predicted_energy_charging_target: [number, string] = [1522, 'u16'];
export const battery_actual_value_power_heating: [number, string] = [1524, 'u16'];
export const battery_prediction_voltage_shortterm_charge: [number, string] = [1526, 'u16'];
export const battery_prediction_voltage_shortterm_discharge: [number, string] = [1528, 'u16'];
export const battery_prediction_voltage_longterm_charge: [number, string] = [1530, 'u16'];
export const battery_prediction_voltage_longterm_discharge: [number, string] = [1532, 'u16'];
export const battery_prediction_duration_charging_minutes: [number, string] = [1534, 'u16'];
export const battery_target_voltage_in_CV_mode: [number, string] = [1536, 'u16'];
export const battery_soc: [number, string] = [1538, 'u16'];
export const battery_soc_hvmax: [number, string] = [1540, 'u16'];
export const battery_soc_hvmin: [number, string] = [1542, 'u16'];
export const battery_capacity_cah: [number, string] = [1544, 'u16'];
export const battery_temperature_HV: [number, string] = [1546, 'i16'];
export const battery_temperature_heat_exchanger: [number, string] = [1548, 'i16'];
export const battery_temperature_max: [number, string] = [1550, 'i16'];
export const battery_temperature_min: [number, string] = [1552, 'i16'];
export const battery_max_charge_amperage: [number, string] = [1554, 'i16'];
export const battery_max_discharge_amperage: [number, string] = [1556, 'i16'];
export const battery_current: [number, string] = [1558, 'i16'];
export const battery_status_error_isolation_external_Bordnetz: [number, string] = [1560, 'u8'];
export const battery_status_error_isolation_internal_Bordnetz: [number, string] = [1561, 'u8'];
export const battery_request_cooling: [number, string] = [1562, 'u8'];
export const battery_status_valve_cooling: [number, string] = [1563, 'u8'];
export const battery_status_error_locking: [number, string] = [1564, 'u8'];
export const battery_status_precharge_locked: [number, string] = [1565, 'u8'];
export const battery_status_disconnecting_switch: [number, string] = [1566, 'u8'];
export const battery_status_emergency_mode: [number, string] = [1567, 'u8'];
export const battery_request_service: [number, string] = [1568, 'u8'];
export const battery_error_emergency_mode: [number, string] = [1569, 'u8'];
export const battery_status_error_disconnecting_switch: [number, string] = [1570, 'u8'];
export const battery_status_warning_isolation: [number, string] = [1571, 'u8'];
export const battery_status_cold_shutoff_valve: [number, string] = [1572, 'u8'];
export const battery_request_open_contactors: [number, string] = [1573, 'u8'];
export const battery_request_open_contactors_instantly: [number, string] = [1574, 'u8'];
export const battery_request_open_contactors_fast: [number, string] = [1575, 'u8'];
export const battery_charging_condition_delta: [number, string] = [1576, 'u8'];
export const battery_status_service_disconnection_plug: [number, string] = [1577, 'u8'];
export const battery_status_measurement_isolation: [number, string] = [1578, 'u8'];
export const battery_request_abort_charging: [number, string] = [1579, 'u8'];
export const battery_prediction_time_end_of_charging_minutes: [number, string] = [1580, 'u8'];
export const battery_request_operating_mode: [number, string] = [1581, 'u8'];
export const battery_request_charging_condition_minimum: [number, string] = [1582, 'u8'];
export const battery_request_charging_condition_maximum: [number, string] = [1583, 'u8'];
export const battery_status_cooling_HV: [number, string] = [1584, 'u8'];
export const battery_status_diagnostics_HV: [number, string] = [1585, 'u8'];
export const battery_status_diagnosis_powertrain_maximum_multiplexer: [number, string] = [1586, 'u8'];
export const battery_status_diagnosis_powertrain_immediate_multiplexer: [number, string] = [1587, 'u8'];
export const battery_ID2: [number, string] = [1588, 'u8'];
export const battery_soh: [number, string] = [1589, 'u8'];
export const message_data: [number, string, number] = [1590, 'u8', 50];
export const next_data: [number, string] = [1640, 'u8'];
export const current_cell_polled: [number, string] = [1641, 'u8'];
