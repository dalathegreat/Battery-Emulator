// CmpSmartCarBattery: 600 bytes; base classes: CanBattery@0
export const CMP_SMART_CAR_BATTERY_FIELDS: ([string, string] | [string, string, number])[] = [
  ['___vptr$Battery', '__vtbl_ptr_type*'],
  ['__defaultRenderer', 'BatteryDefaultRenderer'],
  ['', ' ', 4],
  ['___vptr$Transmitter', '__vtbl_ptr_type*'],
  ['___vptr$CanReceiver', '__vtbl_ptr_type*'],
  ['_can_interface', 'CAN_Interface'],
  ['_initial_speed', 'CAN_Speed'],
  ['renderer', 'CmpSmartCarHtmlRenderer'],
  ['datalayer_battery', 'DATALAYER_BATTERY_TYPE*'],
  ['datalayer_cmpsmart', 'DATALAYER_INFO_CMPSMART*'],
  ['previousMillis10', 'u32'],
  ['previousMillis50', 'u32'],
  ['previousMillis60', 'u32'],
  ['previousMillis100', 'u32'],
  ['previousMillis1000', 'u32'],
  ['precalculated432', 'u8', 16],
  ['CMP_211', 'CAN_frame'],
  ['CMP_351', 'CAN_frame'],
  ['CMP_432', 'CAN_frame'],
  ['CMP_POLL', 'CAN_frame'],
  ['CMP_CLEAR_ALL_DTC', 'CAN_frame'],
  ['vehicle_time_counter', 'u32'],
  ['main_contactor_cycle_count', 'u32'],
  ['QC_contactor_cycle_count', 'u32'],
  ['lifetime_kWh_charged', 'u32'],
  ['lifetime_kWh_discharged', 'u32'],
  ['remaining_energy_Wh', 'u32'],
  ['total_energy_when_full_Wh', 'u32'],
  ['total_coloumb_counting_Ah', 'u32'],
  ['total_coulomb_counting_kWh', 'u32'],
  ['discharge_available_10s_power', 'u16'],
  ['discharge_available_10s_current', 'u16'],
  ['discharge_cont_available_power', 'u16'],
  ['discharge_cont_available_current', 'u16'],
  ['discharge_available_30s_current', 'u16'],
  ['discharge_available_30s_power', 'u16'],
  ['regen_charge_cont_power', 'u16'],
  ['regen_charge_30s_power', 'u16'],
  ['regen_charge_30s_current', 'u16'],
  ['regen_charge_cont_current', 'u16'],
  ['regen_charge_10s_current', 'u16'],
  ['regen_charge_10s_power', 'u16'],
  ['quick_charge_port_voltage', 'u16'],
  ['insulation_resistance_kOhm', 'u16'],
  ['DC_bus_voltage', 'u16'],
  ['charge_max_voltage', 'u16'],
  ['charge_cont_curr_max', 'u16'],
  ['charge_cont_curr_req', 'u16'],
  ['hours_spent_overvoltage', 'u16'],
  ['hours_spent_overtemperature', 'u16'],
  ['hours_spent_undertemperature', 'u16'],
  ['battery_soc', 'u16'],
  ['battery_voltage', 'u16'],
  ['temp', 'u16'],
  ['min_cell_voltage', 'u16'],
  ['max_cell_voltage', 'u16'],
  ['nominal_voltage', 'u16'],
  ['charge_continue_power_limit', 'u16'],
  ['charge_energy_amount_requested', 'u16'],
  ['hours_spent_exceeding_charge_power', 'u16'],
  ['hours_spent_exceeding_discharge_power', 'u16'],
  ['SOC_actual', 'u16'],
  ['battery_temperature_average', 'i16'],
  ['battery_temperature_maximum', 'i16'],
  ['coolant_temperature', 'i16'],
  ['battery_temperature_minimum', 'i16'],
  ['battery_current_dA', 'i16'],
  ['tempval', 'u8'],
  ['startup_increment', 'u8'],
  ['active_DTC_code', 'u8'],
  ['battery_quickcharge_connect_status', 'u8'],
  ['eplug_status', 'u8'],
  ['ev_warning', 'u8'],
  ['battery_state', 'u8'],
  ['battery_fault', 'u8'],
  ['battery_negative_contactor_state', 'u8'],
  ['battery_precharge_contactor_state', 'u8'],
  ['battery_positive_contactor_state', 'u8'],
  ['battery_connect_status', 'u8'],
  ['battery_charging_status', 'u8'],
  ['min_cell_voltage_number', 'u8'],
  ['max_cell_voltage_number', 'u8'],
  ['bulk_SOC_DC_limit', 'u8'],
  ['mux', 'u8'],
  ['startup_counter_432', 'u8'],
  ['counter_10ms', 'u8'],
  ['counter_50ms', 'u8'],
  ['counter_60ms', 'u8'],
  ['counter_100ms', 'u8'],
  ['SOH_internal_resistance', 'u8'],
  ['SOH_estimated', 'u8'],
  ['max_temperature_probe_number', 'u8'],
  ['min_temperature_probe_number', 'u8'],
  ['number_of_temperature_sensors', 'u8'],
  ['number_of_cells', 'u8'],
  ['coolant_temperature_warning', 'u8'],
  ['heater_relay_status', 'u8'],
  ['preheating_status', 'u8'],
  ['thermal_control', 'u8'],
  ['thermal_runaway', 'u8'],
  ['thermal_runaway_module_ID', 'u8'],
  ['HVIL_status', 'u8'],
  ['hardware_fault_status', 'u8'],
  ['insulation_fault', 'u8'],
  ['temperature', 'u8'],
  ['insulation_circuit_status', 'u8'],
  ['plausibility_error', 'u8'],
  ['service_due', 'u8'],
  ['l3_fault', 'u8'],
  ['master_warning', 'u8'],
  ['hvbat_wakeup_state', 'u8'],
  ['alert_frame3', 'u8'],
  ['alert_frame4', 'u8'],
  ['rcd_line_active', 'b'],
  ['power_auth', 'b'],
  ['battery_balancing_active', 'b'],
  ['coolant_alarm', 'b'],
  ['cooling_enabled', 'b'],
  ['battery_minimum_voltage_reached_warning', 'b'],
  ['alert_low_battery_energy', 'b'],
];

export const ___vptr$Battery: [number, string] = [0, '__vtbl_ptr_type*'];
export const __defaultRenderer: [number, string] = [4, 'BatteryDefaultRenderer'];
export const ___vptr$Transmitter: [number, string] = [8, '__vtbl_ptr_type*'];
export const ___vptr$CanReceiver: [number, string] = [12, '__vtbl_ptr_type*'];
export const _can_interface: [number, string] = [16, 'CAN_Interface'];
export const _initial_speed: [number, string] = [20, 'CAN_Speed'];
export const renderer: [number, string] = [24, 'CmpSmartCarHtmlRenderer'];
export const datalayer_battery: [number, string] = [28, 'DATALAYER_BATTERY_TYPE*'];
export const datalayer_cmpsmart: [number, string] = [32, 'DATALAYER_INFO_CMPSMART*'];
export const previousMillis10: [number, string] = [36, 'u32'];
export const previousMillis50: [number, string] = [40, 'u32'];
export const previousMillis60: [number, string] = [44, 'u32'];
export const previousMillis100: [number, string] = [48, 'u32'];
export const previousMillis1000: [number, string] = [52, 'u32'];
export const precalculated432: [number, string, number] = [56, 'u8', 16];
export const CMP_211: [number, string] = [72, 'CAN_frame'];
export const CMP_351: [number, string] = [144, 'CAN_frame'];
export const CMP_432: [number, string] = [216, 'CAN_frame'];
export const CMP_POLL: [number, string] = [288, 'CAN_frame'];
export const CMP_CLEAR_ALL_DTC: [number, string] = [360, 'CAN_frame'];
export const vehicle_time_counter: [number, string] = [432, 'u32'];
export const main_contactor_cycle_count: [number, string] = [436, 'u32'];
export const QC_contactor_cycle_count: [number, string] = [440, 'u32'];
export const lifetime_kWh_charged: [number, string] = [444, 'u32'];
export const lifetime_kWh_discharged: [number, string] = [448, 'u32'];
export const remaining_energy_Wh: [number, string] = [452, 'u32'];
export const total_energy_when_full_Wh: [number, string] = [456, 'u32'];
export const total_coloumb_counting_Ah: [number, string] = [460, 'u32'];
export const total_coulomb_counting_kWh: [number, string] = [464, 'u32'];
export const discharge_available_10s_power: [number, string] = [468, 'u16'];
export const discharge_available_10s_current: [number, string] = [470, 'u16'];
export const discharge_cont_available_power: [number, string] = [472, 'u16'];
export const discharge_cont_available_current: [number, string] = [474, 'u16'];
export const discharge_available_30s_current: [number, string] = [476, 'u16'];
export const discharge_available_30s_power: [number, string] = [478, 'u16'];
export const regen_charge_cont_power: [number, string] = [480, 'u16'];
export const regen_charge_30s_power: [number, string] = [482, 'u16'];
export const regen_charge_30s_current: [number, string] = [484, 'u16'];
export const regen_charge_cont_current: [number, string] = [486, 'u16'];
export const regen_charge_10s_current: [number, string] = [488, 'u16'];
export const regen_charge_10s_power: [number, string] = [490, 'u16'];
export const quick_charge_port_voltage: [number, string] = [492, 'u16'];
export const insulation_resistance_kOhm: [number, string] = [494, 'u16'];
export const DC_bus_voltage: [number, string] = [496, 'u16'];
export const charge_max_voltage: [number, string] = [498, 'u16'];
export const charge_cont_curr_max: [number, string] = [500, 'u16'];
export const charge_cont_curr_req: [number, string] = [502, 'u16'];
export const hours_spent_overvoltage: [number, string] = [504, 'u16'];
export const hours_spent_overtemperature: [number, string] = [506, 'u16'];
export const hours_spent_undertemperature: [number, string] = [508, 'u16'];
export const battery_soc: [number, string] = [510, 'u16'];
export const battery_voltage: [number, string] = [512, 'u16'];
export const temp: [number, string] = [514, 'u16'];
export const min_cell_voltage: [number, string] = [516, 'u16'];
export const max_cell_voltage: [number, string] = [518, 'u16'];
export const nominal_voltage: [number, string] = [520, 'u16'];
export const charge_continue_power_limit: [number, string] = [522, 'u16'];
export const charge_energy_amount_requested: [number, string] = [524, 'u16'];
export const hours_spent_exceeding_charge_power: [number, string] = [526, 'u16'];
export const hours_spent_exceeding_discharge_power: [number, string] = [528, 'u16'];
export const SOC_actual: [number, string] = [530, 'u16'];
export const battery_temperature_average: [number, string] = [532, 'i16'];
export const battery_temperature_maximum: [number, string] = [534, 'i16'];
export const coolant_temperature: [number, string] = [536, 'i16'];
export const battery_temperature_minimum: [number, string] = [538, 'i16'];
export const battery_current_dA: [number, string] = [540, 'i16'];
export const tempval: [number, string] = [542, 'u8'];
export const startup_increment: [number, string] = [543, 'u8'];
export const active_DTC_code: [number, string] = [544, 'u8'];
export const battery_quickcharge_connect_status: [number, string] = [545, 'u8'];
export const eplug_status: [number, string] = [546, 'u8'];
export const ev_warning: [number, string] = [547, 'u8'];
export const battery_state: [number, string] = [548, 'u8'];
export const battery_fault: [number, string] = [549, 'u8'];
export const battery_negative_contactor_state: [number, string] = [550, 'u8'];
export const battery_precharge_contactor_state: [number, string] = [551, 'u8'];
export const battery_positive_contactor_state: [number, string] = [552, 'u8'];
export const battery_connect_status: [number, string] = [553, 'u8'];
export const battery_charging_status: [number, string] = [554, 'u8'];
export const min_cell_voltage_number: [number, string] = [555, 'u8'];
export const max_cell_voltage_number: [number, string] = [556, 'u8'];
export const bulk_SOC_DC_limit: [number, string] = [557, 'u8'];
export const mux: [number, string] = [558, 'u8'];
export const startup_counter_432: [number, string] = [559, 'u8'];
export const counter_10ms: [number, string] = [560, 'u8'];
export const counter_50ms: [number, string] = [561, 'u8'];
export const counter_60ms: [number, string] = [562, 'u8'];
export const counter_100ms: [number, string] = [563, 'u8'];
export const SOH_internal_resistance: [number, string] = [564, 'u8'];
export const SOH_estimated: [number, string] = [565, 'u8'];
export const max_temperature_probe_number: [number, string] = [566, 'u8'];
export const min_temperature_probe_number: [number, string] = [567, 'u8'];
export const number_of_temperature_sensors: [number, string] = [568, 'u8'];
export const number_of_cells: [number, string] = [569, 'u8'];
export const coolant_temperature_warning: [number, string] = [570, 'u8'];
export const heater_relay_status: [number, string] = [571, 'u8'];
export const preheating_status: [number, string] = [572, 'u8'];
export const thermal_control: [number, string] = [573, 'u8'];
export const thermal_runaway: [number, string] = [574, 'u8'];
export const thermal_runaway_module_ID: [number, string] = [575, 'u8'];
export const HVIL_status: [number, string] = [576, 'u8'];
export const hardware_fault_status: [number, string] = [577, 'u8'];
export const insulation_fault: [number, string] = [578, 'u8'];
export const temperature: [number, string] = [579, 'u8'];
export const insulation_circuit_status: [number, string] = [580, 'u8'];
export const plausibility_error: [number, string] = [581, 'u8'];
export const service_due: [number, string] = [582, 'u8'];
export const l3_fault: [number, string] = [583, 'u8'];
export const master_warning: [number, string] = [584, 'u8'];
export const hvbat_wakeup_state: [number, string] = [585, 'u8'];
export const alert_frame3: [number, string] = [586, 'u8'];
export const alert_frame4: [number, string] = [587, 'u8'];
export const rcd_line_active: [number, string] = [588, 'b'];
export const power_auth: [number, string] = [589, 'b'];
export const battery_balancing_active: [number, string] = [590, 'b'];
export const coolant_alarm: [number, string] = [591, 'b'];
export const cooling_enabled: [number, string] = [592, 'b'];
export const battery_minimum_voltage_reached_warning: [number, string] = [593, 'b'];
export const alert_low_battery_energy: [number, string] = [594, 'b'];
