// BydAtto3Extended
//
// Port of the legacy C++ BYD-ATTO-3-HTML.h renderer (BydAtto3HtmlRenderer)
// to the Preact frontend. All display values are read from the /api/batext
// blob via the generated DATALAYER_INFO_BYDATTO3 offset table - no hardcoded
// offsets here. User-editable values are written through the standard
// settings API (/api/internal/settings, which persists them) and the iso
// enable/disable commands through the generic POST routes, so the component
// needs no battery-specific GET endpoints.
//
// Battery 1 vs 2: /api/batext currently serves battery 1 only; the component
// accepts `battery={1|2}` and switches the settings keys it writes
// (BYDAUTOCALEN/BYDAUTOCALEN2 etc.) so it is ready when a second-blob
// endpoint exists. The iso controls are shared (one setting, both packs).

import { useEffect, useState } from "preact/hooks";

import * as L from "./datalayer/DATALAYER_INFO_BYDATTO3.ts";
import { lookup } from "./consts.ts";
import { apiPost, refreshApi } from "../utils/api.tsx";

// Persisted settings keys, per battery position (see webserver_settings.cpp).
const KEYS = {
  1: { enabled: "BYDAUTOCALEN", drift: "BYDAUTOCALDRIFT", targetSOC: "TMP_CALTARGETSOC", targetAH: "TMP_CALTARGETAH" },
  2: { enabled: "BYDAUTOCALEN2", drift: "BYDAUTOCALDRFT2", targetSOC: "TMP_CALTARGETSOC2", targetAH: "TMP_CALTARGETAH2" },
} as const;

const CONTACTOR_CONTROL_STATE = [
  "Closing",
  "Closed (live)",
  "Preparing to open",
  "Opening",
  "Standby / idle",
  "Open requested",
  "Open (settling)",
  "Held open (fault / e-stop / startup)",
];

// 0x344 byte0 state table, read verbatim (see the C++ renderer comments).
function packModeName(feedback: number): string {
  switch (feedback) {
    case 0x00: return "Disconnected";
    case 0x02: return "Open standby";
    case 0x42: return "Precharging";
    case 0x80: return "Closed, HV inactive";
    case 0x84: return "Closed idle, HV active";
    case 0x81: return "Charging, car off";
    case 0x85: return "Charging, HV active";
    case 0x82: return "Drive-ready pending";
    case 0x86: return "Drive ready";
    default: {
      let name = "Closed idle";
      if (!(feedback & 0x80)) name = "Disconnected";
      else if (feedback & 0x01) name = "Charging";
      else if (feedback & 0x02) name = "Drive";
      return `${name} (0x${feedback.toString(16).padStart(2, "0").toUpperCase()})`;
    }
  }
}

const serviceModeName = ["No command ran yet", "REJECTED", "APPROVED!"];

function statusSpan(ok: boolean | null, yes: string, no: string, unknown?: string) {
  if (ok === null) return <span class="text-muted">{unknown ?? "Unknown"}</span>;
  return ok ? <span class="text-ok">{yes}</span> : <span class="text-warn">{no}</span>;
}

export function BydAtto3Extended({ view, battery = 1, cells }: { view: DataView; battery?: 1 | 2; cells?: number }) {
  const get = lookup(view);
  const keys = KEYS[battery];

  // ---- Live fields from the /api/batext blob (fresh every poll) ----

  const contactor_control_state = get(L.contactor_control_state) as number;
  const contactor_feedback = get(L.contactor_feedback) as number;
  const discharge_status = get(L.discharge_status) as number;
  const contactor_main_closed = get(L.contactor_main_closed) as number !== 0;
  const contactor_precharging = get(L.contactor_precharging) as number !== 0;
  const contactor_hv_active = get(L.contactor_hv_active) as number !== 0;

  const soc_measured = (get(L.SOC_highprec) as number) * 0.1;
  const soc_polled = get(L.SOC_polled) as number;
  const pack_voltage_dV = get(L.pack_voltage_dV) as number;
  const pack_voltage = pack_voltage_dV > 0 ? pack_voltage_dV / 10.0 : null;
  const temperatures = get(L.battery_temperatures) as number[];
  const max_discharge_power = (get(L.dischargePower) as number) * 0.1;
  const max_charge_power = (get(L.chargePower) as number) * 0.1;
  const total_charged_kwh = get(L.total_charged_kwh) as number;
  const total_discharged_kwh = get(L.total_discharged_kwh) as number;
  const total_charged_ah = get(L.total_charged_ah) as number;
  const total_discharged_ah = get(L.total_discharged_ah) as number;
  const charge_times = get(L.charge_times) as number;
  const times_full_power = get(L.times_full_power) as number;
  const seed = get(L.seed) as number;
  const solvedKey = get(L.solvedKey) as number;
  const servicemode = get(L.servicemode) as number;
  const capacity_original = (get(L.BMS_capacity_original_calibration) as number) / 100;
  const capacity_current = (get(L.BMS_capacity_current_calibration) as number) / 100;
  const soc_original = get(L.BMC_SOC_original_calibration) as number;
  const soc_current = get(L.BMC_SOC_current_calibration) as number;

  const auto_cal_enabled = get(L.auto_calibrate_soc_enabled) as number !== 0;
  const drift_threshold = get(L.auto_calibrate_soc_drift_percent) as number;
  const cal_target_soc = get(L.calibrationTargetSOC) as number;
  const cal_target_ah = get(L.calibrationTargetAH) as number;

  const dwell_sec = Math.floor((get(L.autocal_dwell_accumulated_ms) as number) / 1000);
  const dwell_min = Math.floor(dwell_sec / 60);
  const dwell_rem = dwell_sec % 60;
  const grace_sec = Math.floor((get(L.autocal_grace_timer_ms) as number) / 1000);
  const autocal_current_A = (get(L.autocal_current_dA) as number) / 10.0;
  const current_direction = autocal_current_A < 0 ? "discharge" : autocal_current_A > 0 ? "charge" : "idle";
  const dwell_done = get(L.autocal_crit_dwell) as number !== 0;
  const crit_taper = get(L.autocal_crit_taper) as number !== 0;
  const crit_low_current = get(L.autocal_crit_low_current) as number !== 0;
  const crit_drift = get(L.autocal_crit_drift) as number !== 0;
  const crit_cooldown_ready = get(L.autocal_crit_cooldown_ready) as number !== 0;
  const crit_contactors = get(L.autocal_crit_contactors) as number !== 0;
  const drift_percent = get(L.autocal_drift_percent) as number;

  const iso_status_valid = get(L.iso_status_valid) as number !== 0;
  const iso_active = get(L.iso_measurement_active) as number !== 0;
  const insulation_valid = get(L.insulation_valid) as number !== 0;
  const insulation_ohm_per_volt = get(L.insulation_ohm_per_volt) as number;
  const iso_command_status = get(L.iso_command_status) as number;
  const keep_iso_disabled = get(L.keep_iso_disabled) as number !== 0;

  const iso_kohm = insulation_valid && pack_voltage !== null
    ? insulation_ohm_per_volt * pack_voltage / 1000.0
    : null;

  // ---- Editable controls ----

  // Drift threshold: local input state seeded from the blob; write is
  // persisted via the settings API.
  const [driftInput, setDriftInput] = useState<string>(String(drift_threshold));
  useEffect(() => { setDriftInput(String(drift_threshold)); }, [drift_threshold]);

  const postSetting = async (key: string, value: string) => {
    try {
      await apiPost("/api/internal/settings", { [key]: value });
      refreshApi();
    } catch (e: any) {
      alert(e.message || "Update failed. Please try again.");
    }
  };

  const editTarget = (label: string, key: string, min: number, max: number) => {
    const value = window.prompt(label);
    if (value === null) return;
    const num = parseFloat(value);
    if (isNaN(num) || num < min || num > max) {
      alert("Invalid value. Please enter a value between " + min + " and " + max + ".");
      return;
    }
    postSetting(key, String(num));
  };

  const isoCommand = async (enable: boolean) => {
    try {
      await fetch((import.meta.env.VITE_API_BASE || "") + `/api/bydatto3/iso/${enable ? "enable" : "disable"}`, { method: "POST" });
      // Status (iso_command_status / iso_measurement_active) arrives with the
      // next /api/batext poll; trigger one immediately.
      refreshApi();
    } catch (e: any) {
      alert(e.message || "Update failed. Please try again.");
    }
  };

  return <div>
    <div class="panel">
      <h3>Contactors &amp; BMS state</h3>
      <h4>BE contactor state: {CONTACTOR_CONTROL_STATE[contactor_control_state] ?? "Unknown"}</h4>
      <h4>Main contactors: {contactor_main_closed ? "Closed — battery connected" : "Open — battery disconnected"}</h4>
      <h4>Precharge state: {contactor_precharging ? "Active" : "Idle"}</h4>
      {/* Bit2 (0x04) = car on/off, not literal HV-bus energisation. */}
      <h4>HV active: {contactor_hv_active ? "Yes" : "No"}</h4>
      <h4>BMS pack mode: {packModeName(contactor_feedback)}</h4>
      <h4>
        BMS raw status: mode 0x{contactor_feedback.toString(16).padStart(2, "0").toUpperCase()}, state {discharge_status}
      </h4>
    </div>

    <div class="panel">
      <h3>Pack measurements</h3>
      <h4>Detected cells: {cells !== undefined ? cells : "—"}</h4>
      <h4>SOC measured: {soc_measured.toFixed(1)}%</h4>
      <h4>SOC 0x444: {soc_polled}%</h4>
      <h4>Pack voltage: {pack_voltage !== null ? pack_voltage.toFixed(1) + " V" : "Not received"}</h4>
      {temperatures.map((t, i) =>
        t !== 215 && <h4 key={i}>Temperature sensor {i + 1}: {t} °C</h4>
      )}
      <h4>Max discharge power: {max_discharge_power.toFixed(1)} kW</h4>
      <h4>Max charge (regen) power: {max_charge_power.toFixed(1)} kW</h4>
      <h4>Total charged: {total_charged_kwh} kWh</h4>
      <h4>Total discharged: {total_discharged_kwh} kWh</h4>
      <h4>Total charged: {total_charged_ah} Ah</h4>
      <h4>Total discharged: {total_discharged_ah} Ah</h4>
      <h4>Charge times: {charge_times}</h4>
      <h4>Times of full power: {times_full_power}</h4>
      <h4>Min cell voltage number: {get(L.BMS_min_cell_voltage_number)}</h4>
      <h4>Max cell voltage number: {get(L.BMS_max_cell_voltage_number)}</h4>
      <h4>Min temp module number: {get(L.BMS_min_temp_module_number)}</h4>
      <h4>Max temp module number: {get(L.BMS_max_temp_module_number)}</h4>
      <h4>Seed: {seed} SolvedKey: {solvedKey}</h4>
      <h4>ServiceMode: {serviceModeName[servicemode] ?? "Unknown"}</h4>
      <h4>Capacity original: {capacity_original.toFixed(1)} AH</h4>
      <h4>Capacity current: {capacity_current.toFixed(1)} AH</h4>
      <h4>SOC original: {soc_original}%</h4>
      <h4>SOC current: {soc_current}%</h4>
    </div>

    <div class="panel">
      <h3>Auto-calibration</h3>
      <h4>
        Auto-calibrate SOC to 100% when full:{" "}
        <input
          type="checkbox"
          checked={auto_cal_enabled}
          onChange={(ev) => postSetting(keys.enabled, (ev.currentTarget.checked ? 1 : 0).toString())}
        />{" "}
        (default ON)
      </h4>
      <h4>
        Auto-calibrate trigger drift:{" "}
        <input
          type="number"
          min="1"
          max="20"
          value={driftInput}
          onInput={(ev) => setDriftInput(ev.currentTarget.value)}
        />{" "}
        %{" "}
        <button class="button" disabled={Number(driftInput) < 1 || Number(driftInput) > 20}
          onClick={() => postSetting(keys.drift, driftInput)}>Save Drift %</button>
      </h4>

      <table class="stat-table">
        <tr><td>Contactors:</td><td>{statusSpan(crit_contactors, "OK", "Open")}</td></tr>
        <tr><td>Full / In taper?</td><td>{statusSpan(crit_taper, "Yes", "No")}</td></tr>
        <tr><td>Battery current:</td><td>{autocal_current_A.toFixed(1)} A ({current_direction})</td></tr>
        <tr>
          <td>Current in range:</td>
          <td>
            {!crit_taper
              ? <span class="text-muted">Waiting for taper</span>
              : crit_low_current
                ? <span class="text-ok">Yes</span>
                : <span class="text-warn">No — {grace_sec}s / 60s</span>}
          </td>
        </tr>
        <tr>
          <td>Dwell time:</td>
          <td>
            {dwell_done ? <span class="text-ok">{dwell_min}m {dwell_rem}s / 10m</span>
                        : <span>{dwell_min}m {dwell_rem}s / 10m</span>}
          </td>
        </tr>
        <tr>
          <td>SOC drift:</td>
          <td>
            {crit_drift
              ? <span class="text-ok">{drift_percent.toFixed(1)}% / threshold {drift_threshold}%</span>
              : <span>{drift_percent.toFixed(1)}% / threshold {drift_threshold}%</span>}
          </td>
        </tr>
        <tr><td>Cooldown:</td><td>{statusSpan(crit_cooldown_ready, "Ready", "Waiting")}</td></tr>
      </table>

      <h4>
        Calibration target SOC: {cal_target_soc}%{" "}
        <button class="button"
          onClick={() => editTarget("Enter calibration target SOC (0 to 100):", keys.targetSOC, 0, 100)}>Edit</button>
      </h4>
      <h4>
        Calibration target capacity: {cal_target_ah} AH{" "}
        <button class="button"
          onClick={() => editTarget("Enter calibration target AH:", keys.targetAH, 1, 1000)}>Edit</button>
      </h4>
    </div>

    <div class="panel">
      <h3>Isolation resistance monitor</h3>
      <table class="stat-table">
        <tr>
          <td>Monitoring:</td>
          <td>
            {!iso_status_valid
              ? <span class="text-muted">Unknown</span>
              : !contactor_main_closed
                ? <span class="text-muted">Inactive (pack open)</span>
                : iso_active
                  ? <span class="text-ok">On</span>
                  : <span class="text-error">Off</span>}
          </td>
        </tr>
        <tr>
          <td>Insulation resistance:</td>
          <td>
            {iso_kohm !== null
              ? iso_kohm.toFixed(1) + " kΩ (" + insulation_ohm_per_volt + " Ω/V)"
              : <span class="text-muted">Not received</span>}
          </td>
        </tr>
        {(iso_command_status === 1 || iso_command_status === 3 || iso_command_status === 4) && (
          <tr>
            <td>Last command:</td>
            <td>
              {iso_command_status === 1 && <span class="text-warn">Sending…</span>}
              {iso_command_status === 3 && <span class="text-error">Rejected</span>}
              {iso_command_status === 4 && <span class="text-error">No response</span>}
            </td>
          </tr>
        )}
      </table>
      <h4>
        Keep disabled at boot:{" "}
        <input
          type="checkbox"
          checked={keep_iso_disabled}
          onChange={(ev) => postSetting("BYDKEEPISOOFF", (ev.currentTarget.checked ? 1 : 0).toString())}
        />
      </h4>
      <div class="button-row">
        <button class="button" onClick={() => isoCommand(true)}>Enable monitoring</button>{" "}
        <button class="button" onClick={() => isoCommand(false)}>Disable monitoring</button>
      </div>
    </div>
  </div>;
}
