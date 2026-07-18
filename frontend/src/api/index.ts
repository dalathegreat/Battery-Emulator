import { BatteryApi } from "./battery.js";
import { CanlogApi } from "./canlog.js";
import { DashboardApi } from "./dashboard.js";
import { DebuglogApi } from "./debuglog.js";
import { EventsApi } from "./events.js";
import { SettingsApi } from "./settings.js";
import { SystemApi } from "./system.js";

export const api = {
  battery: new BatteryApi(),
  dashboard: new DashboardApi(),
  debuglog: new DebuglogApi(),
  system: new SystemApi(),
  settings: new SettingsApi(),
  events: new EventsApi(),
  canlog: new CanlogApi(),
};

export type Api = typeof api;
