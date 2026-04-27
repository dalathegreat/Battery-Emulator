/** Pack snapshot from /api/battery/status */
export interface BatteryPackStatus {
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
}

export interface BatteryStatusResponse {
  battery: BatteryPackStatus;
  battery2: BatteryPackStatus;
  battery3: BatteryPackStatus;
  system: {
    bms_status: string;
    pause_status: string;
    emulator_status: string;
    cpu_temp_C: number;
    uptime_s: number;
    free_heap: number;
  };
  charger: {
    hv_enabled: boolean;
    aux12v_enabled: boolean;
    setpoint_HV_V: number;
    setpoint_HV_A: number;
    stat_HV_V: number;
    stat_HV_A: number;
  };
}

export interface CellEntry {
  voltage_mV: number;
  balancing: boolean;
}

export interface BatteryCellGroup {
  present: boolean;
  number_of_cells: number;
  cells: CellEntry[];
}

export interface CellMonitorResponse {
  packs: [BatteryCellGroup, BatteryCellGroup, BatteryCellGroup];
}

export interface BatteryCommandDef {
  id: string;
  label: string;
  confirm: string | null;
  batteries: number[];
}

export interface BatteryCommandsResponse {
  commands: BatteryCommandDef[];
}
