import type {
  BatteryCommandsResponse,
  BatteryStatusResponse,
  CellMonitorResponse,
} from "../types/battery.types.js";
import { TESLA_MODEL_3Y_TYPE } from "../components/cellmonitor/tesla-3y-layout.js";

function pack(partial: Partial<import("../types/battery.types.js").BatteryPackStatus>) {
  return {
    present: true,
    voltage_V: 353.8122,
    current_A: -12.4,
    reported_current_A: -12.4,
    power_W: -4760,
    soc_reported_pct: 72.5,
    soc_real_pct: 74.0,
    soh_pct: 98.2,
    temp_min_C: 19.0,
    temp_max_C: 20.0,
    cell_max_mV: 3704,
    cell_min_mV: 3040,
    cell_delta_mV: 664,
    remaining_Wh: 21000,
    reported_remaining_Wh: 20500,
    total_Wh: 30000,
    max_charge_power_W: 11000,
    max_discharge_power_W: 11000,
    balancing_status: "Ready",
    balancing_active_cells: 0,
    bms_status: "ACTIVE",
    can_alive: true,
    number_of_cells: 96,
    ...partial,
  };
}

export function mockBatteryStatus(): BatteryStatusResponse {
  return {
    battery: pack({}),
    battery2: pack({
      present: false,
      voltage_V: 0,
      current_A: 0,
      power_W: 0,
      soc_reported_pct: 0,
      soc_real_pct: 0,
      number_of_cells: 0,
    }),
    battery3: pack({
      present: false,
      voltage_V: 0,
      current_A: 0,
      power_W: 0,
      soc_reported_pct: 0,
      soc_real_pct: 0,
      number_of_cells: 0,
    }),
    system: {
      bms_status: "ACTIVE",
      pause_status: "Running",
      emulator_status: "OK",
      cpu_temp_C: 42,
      uptime_s: 86400,
      free_heap: 180000,
    },
    charger: {
      hv_enabled: false,
      aux12v_enabled: true,
      setpoint_HV_V: 0,
      setpoint_HV_A: 0,
      stat_HV_V: 0,
      stat_HV_A: 0,
    },
  };
}

/** 96 bricks with a dramatic min (#2) and max (#26) like ScanMyTesla demos. */
export function mockCellMonitor(): CellMonitorResponse {
  const n = 96;
  const cells = Array.from({ length: n }, (_, i) => {
    let voltage_mV = 3690 + ((i * 7) % 17) - 8;
    if (i === 1) voltage_mV = 3040; // min
    if (i === 25) voltage_mV = 3704; // max
    if (i === 44) voltage_mV = 3655;
    return {
      voltage_mV,
      balancing: i === 44 || i === 72,
      capacity_Ah: 110 + ((i * 3) % 9) / 10,
    };
  });
  const emptyPack = {
    present: false,
    number_of_cells: 0,
    cells: [] as { voltage_mV: number; balancing: boolean; capacity_Ah?: number }[],
  };
  return {
    packs: [
      { present: true, number_of_cells: n, cells },
      { ...emptyPack },
      { ...emptyPack },
    ],
  };
}

export function mockBatteryCommands(): BatteryCommandsResponse {
  return {
    commands: [
      {
        id: "resetBMS",
        label: "BMS Reset",
        confirm: "Reset the BMS?",
        batteries: [0],
      },
      {
        id: "resetSOC",
        label: "SOC Reset",
        confirm: "Reset SOC?",
        batteries: [0, 1],
      },
    ],
  };
}

/** Re-export for settings mock. */
export { TESLA_MODEL_3Y_TYPE };
