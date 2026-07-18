/** GET /api/dashboard */
export interface DashboardPack {
  present: boolean;
  voltage_V: number;
  current_A: number;
  reported_current_A: number;
  power_W: number;
  soc_reported_pct: number;
  soc_real_pct: number;
  soh_pct: number;
  temp_min_C: number;
  temp_max_C: number;
  cell_max_mV: number;
  cell_min_mV: number;
  cell_delta_mV: number;
  remaining_Wh: number;
  reported_remaining_Wh: number;
  total_Wh: number;
  max_charge_power_W: number;
  max_discharge_power_W: number;
  balancing_status: string;
  balancing_active_cells: number;
  bms_status: string;
  can_alive: boolean;
  number_of_cells: number;
  reported_total_Wh?: number;
  max_charge_current_A?: number;
  max_discharge_current_A?: number;
  soc_scaling_active?: boolean;
  flow?: string;
  remote_limit_discharge?: boolean;
  remote_limit_charge?: boolean;
  user_limit_discharge?: boolean;
  user_limit_charge?: boolean;
  inverter_limit_discharge?: boolean;
  inverter_limit_charge?: boolean;
  real_bms_status?: number;
}

export interface DashboardWifi {
  ssid: string;
  connected: boolean;
  rssi?: number;
  channel?: number;
  hostname?: string;
  ip?: string;
  state_text: string;
}

export interface DashboardPerformance {
  free_heap: number;
  max_alloc_heap: number;
  flash_mode: string;
  flash_mb: number;
  core_task_max_us: number;
  core_task_10s_max_us: number;
  mqtt_task_10s_max_us: number;
  wifi_task_10s_max_us: number;
  time_snap_10ms_us: number;
  time_snap_values_us: number;
  time_snap_comm_us: number;
  time_snap_cantx_us: number;
}

export interface DashboardContactor {
  pause_status_text: string;
  emulator_pause_request_on: boolean;
  equipment_stop_active: boolean;
  emulator_allows_contactor_close: boolean;
  inverter_allows_contactor_closing: boolean;
  battery2_allowed_contactor_closing: boolean;
  contactor_control_enabled: boolean;
  contactor_control_double_battery: boolean;
  contactors_engaged: boolean;
  contactors_battery2_engaged: boolean;
  precharge_status: number;
  pwm_contactor_control: boolean;
  second_battery_contactor_pin_high?: boolean;
}

export interface DashboardChargerDetail {
  hv_enabled: boolean;
  aux12v_enabled: boolean;
  output_power_W: number;
  efficiency_supported: boolean;
  efficiency_pct?: number;
  hvdc_V: number;
  hvdc_A: number;
  lvdc_V: number;
  lvdc_A: number;
  ac_V: number;
  ac_A: number;
}

export interface DashboardUi {
  web_logging_active: boolean;
  sd_logging_active: boolean;
  webserver_auth: boolean;
  emulator_status: string;
}

export interface DashboardResponse {
  version: string;
  hardware: string;
  uptime_human: string;
  uptime_s: number;
  cpu_temp_C: number;
  wifi: DashboardWifi;
  performance?: DashboardPerformance;
  components: Record<string, string | boolean | undefined>;
  packs: DashboardPack[];
  contactor: DashboardContactor;
  charger_detail?: DashboardChargerDetail;
  ui: DashboardUi;
}
