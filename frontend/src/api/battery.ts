import { xget, xhttp } from "../xFetch.js";
import type {
  BatteryCommandsResponse,
  BatteryStatusResponse,
  CellMonitorResponse,
} from "../types/battery.types.js";
import type { BatteryExtraResponse } from "../types/battery-extra.types.js";

export class BatteryApi {
  async status() {
    return xget<BatteryStatusResponse>("/api/battery/status");
  }

  async extra() {
    return xget<BatteryExtraResponse>("/api/battery/extra");
  }

  async cellmonitor() {
    return xget<CellMonitorResponse>("/api/battery/cellmonitor");
  }

  async commands() {
    return xget<BatteryCommandsResponse>("/api/battery/commands");
  }

  /** Body is battery index character: "0" | "1" | "2" */
  async command(cmd: string, batteryIndex = 0) {
    return xhttp<{ ok?: boolean }>(
      `/${cmd}`,
      "PUT",
      String(batteryIndex)
    );
  }
}
