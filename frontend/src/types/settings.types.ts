/** One entry for HTML &lt;select&gt; (from firmware names). */
export interface SettingsSelectOption {
  value: number;
  label: string;
}

export interface SettingsSelectOptions {
  battery?: SettingsSelectOption[];
  inverter?: SettingsSelectOption[];
  charger?: SettingsSelectOption[];
  chemistry?: SettingsSelectOption[];
  comm?: SettingsSelectOption[];
  shunt?: SettingsSelectOption[];
  eqstop?: SettingsSelectOption[];
  ctatten?: SettingsSelectOption[];
}

/** NVM + live datalayer fields from /api/settings */
export interface SettingsResponse {
  bool: Record<string, boolean>;
  uint: Record<string, number>;
  string: Record<string, string>;
  select: Record<string, number>;
  /** Populated by firmware: human-readable choices for enum-like selects */
  select_options?: SettingsSelectOptions;
  string_extra: Record<string, string>;
  mqtt_publish_s: number;
  datalayer: {
    total_capacity_Wh: number;
    max_charge_A: number;
    max_discharge_A: number;
    soc_max_pct: number;
    soc_min_pct: number;
    charge_voltage_V: number;
    discharge_voltage_V: number;
    soc_scaling_active: boolean;
    voltage_limits_active: boolean;
    manual_balancing: boolean;
    sofar_battery_id: number;
  };
}
