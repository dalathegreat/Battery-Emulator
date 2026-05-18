export interface BatteryExtraRow {
  label: string;
  value: string;
}

export interface BatteryExtraEntry {
  index: number;
  rows: BatteryExtraRow[];
}

export interface BatteryExtraResponse {
  batteries: BatteryExtraEntry[];
}
