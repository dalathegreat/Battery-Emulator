import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { api } from "../api/index.js";
import type { DashboardPack, DashboardResponse } from "../types/dashboard.types.js";
import type { SettingsResponse } from "../types/settings.types.js";

@customElement("app-dashboard")
export class AppDashboard extends LitElement {
  @state() private dash: DashboardResponse | null = null;
  @state() private limits: SettingsResponse["datalayer"] | null = null;
  @state() private err = "";
  @state() private loading = true;
  @state() private actionMsg = "";

  static styles = css`
    :host {
      display: block;
      color: #fff;
      font-family: system-ui, sans-serif;
    }
    .card {
      background: #303e47;
      border-radius: 12px;
      padding: 1rem;
      margin-bottom: 1rem;
    }
    h2,
    h3 {
      margin-top: 0;
    }
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(160px, 1fr));
      gap: 0.75rem;
    }
    .kv {
      background: #1e2c33;
      padding: 0.5rem 0.75rem;
      border-radius: 8px;
    }
    .k {
      font-size: 0.75rem;
      opacity: 0.8;
    }
    .v {
      font-weight: 600;
    }
    .err {
      color: #f88;
    }
    .ok {
      color: #8c8;
      font-size: 14px;
    }
    button,
    .btn {
      background: #505e67;
      color: #fff;
      border: none;
      padding: 0.5rem 1rem;
      border-radius: 8px;
      cursor: pointer;
      margin: 0.25rem 0.25rem 0.25rem 0;
      text-decoration: none;
      display: inline-block;
      font-size: 14px;
    }
    button.danger {
      background: #7a3030;
    }
    button.warn {
      background: #665020;
    }
    .row {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      gap: 0.5rem;
      margin: 0.35rem 0;
    }
    input[type="number"] {
      width: 5.5rem;
      padding: 0.25rem 0.4rem;
      border-radius: 6px;
      border: 1px solid #555;
      background: #1a252b;
      color: #fff;
    }
    .tools a {
      color: #9cf;
    }
  `;

  connectedCallback(): void {
    super.connectedCallback();
    this.load();
  }

  async load() {
    this.loading = true;
    this.err = "";
    const [dr, sr] = await Promise.all([api.dashboard.get(), api.settings.get()]);
    const [, de] = dr;
    const [st, se] = sr;
    if (de) this.err = String(de.error_msg || de);
    this.dash = de ? null : dr[0];
    this.limits = se ? null : st.datalayer;
    this.loading = false;
  }

  private async act(url: string) {
    this.actionMsg = "";
    const [, err] = await api.system.simpleGet(url);
    this.actionMsg = err ? String(err.error_msg || err) : "OK";
    await this.load();
  }

  private renderPack(ix: number, p: DashboardPack) {
    const label = `Battery ${ix + 1}`;
    if (!p.present) {
      return html`<div class="card"><h3>${label}</h3><p>Not present</p></div>`;
    }
    return html`
      <div class="card">
        <h3>${label}</h3>
        <div class="grid">
          <div class="kv"><div class="k">Voltage</div><div class="v">${p.voltage_V.toFixed(1)} V</div></div>
          <div class="kv"><div class="k">Current</div><div class="v">${p.current_A.toFixed(1)} A</div></div>
          <div class="kv"><div class="k">Power</div><div class="v">${p.power_W} W</div></div>
          <div class="kv"><div class="k">Flow</div><div class="v">${p.flow ?? "—"}</div></div>
          <div class="kv"><div class="k">SOC (reported)</div><div class="v">${p.soc_reported_pct.toFixed(1)} %</div></div>
          <div class="kv"><div class="k">SOC (real)</div><div class="v">${p.soc_real_pct.toFixed(1)} %</div></div>
          <div class="kv"><div class="k">SOH</div><div class="v">${p.soh_pct.toFixed(1)} %</div></div>
          <div class="kv"><div class="k">Temp</div><div class="v">${p.temp_min_C.toFixed(1)} – ${p.temp_max_C.toFixed(1)} °C</div></div>
          <div class="kv"><div class="k">Cells Δ</div><div class="v">${p.cell_delta_mV} mV</div></div>
          <div class="kv"><div class="k">Balancing</div><div class="v">${p.balancing_status} (${p.balancing_active_cells})</div></div>
          <div class="kv"><div class="k">BMS</div><div class="v">${p.bms_status}</div></div>
          <div class="kv"><div class="k">CAN alive</div><div class="v">${p.can_alive ? "yes" : "no"}</div></div>
          ${p.reported_total_Wh != null
            ? html`<div class="kv">
                <div class="k">Reported total Wh</div>
                <div class="v">${p.reported_total_Wh}</div>
              </div>`
            : null}
          ${p.max_charge_current_A != null
            ? html`<div class="kv">
                <div class="k">Max chg / dchg A</div>
                <div class="v">${p.max_charge_current_A.toFixed(1)} / ${p.max_discharge_current_A?.toFixed(1) ?? "—"}</div>
              </div>`
            : null}
        </div>
        ${ix === 0 && this.limits
          ? html`
              <h4>Live limits (pack 1)</h4>
              <div class="row">
                <span>Total Wh</span>
                <input type="number" id="wh_${ix}" .value=${String(p.total_Wh)} />
                <button type="button" @click=${() => this.applyWh(ix)}>Apply</button>
              </div>
              <div class="row">
                <span>SOC max %</span>
                <input type="number" id="smax_${ix}" step="0.1" .value=${String(this.limits.soc_max_pct)} />
                <button type="button" @click=${() => this.applySocMax(ix)}>Apply</button>
              </div>
              <div class="row">
                <span>SOC min %</span>
                <input type="number" id="smin_${ix}" step="0.1" .value=${String(this.limits.soc_min_pct)} />
                <button type="button" @click=${() => this.applySocMin(ix)}>Apply</button>
              </div>
              <div class="row">
                <span>Max charge A</span>
                <input type="number" id="cmax_${ix}" step="0.1" .value=${String(this.limits.max_charge_A)} />
                <button type="button" @click=${() => this.applyMaxCharge(ix)}>Apply</button>
              </div>
              <div class="row">
                <span>Max discharge A</span>
                <input type="number" id="dmax_${ix}" step="0.1" .value=${String(this.limits.max_discharge_A)} />
                <button type="button" @click=${() => this.applyMaxDischarge(ix)}>Apply</button>
              </div>
              <div class="row">
                <span>Scaled SOC</span>
                <button type="button" @click=${() => this.act(`/updateUseScaledSOC?value=${p.soc_scaling_active ? 0 : 1}`)}>
                  Toggle (${p.soc_scaling_active ? "on" : "off"})
                </button>
              </div>
            `
          : null}
      </div>
    `;
  }

  private val(id: string): string {
    const el = this.shadowRoot?.querySelector<HTMLInputElement>(`#${CSS.escape(id)}`);
    return el?.value ?? "";
  }

  private async applyWh(_ix: number) {
    const v = this.val(`wh_${_ix}`);
    await this.act(`/updateBatterySize?value=${encodeURIComponent(v)}`);
  }
  private async applySocMax(_ix: number) {
    const v = this.val(`smax_${_ix}`);
    await this.act(`/updateSocMax?value=${encodeURIComponent(v)}`);
  }
  private async applySocMin(_ix: number) {
    const v = this.val(`smin_${_ix}`);
    await this.act(`/updateSocMin?value=${encodeURIComponent(v)}`);
  }
  private async applyMaxCharge(_ix: number) {
    const v = this.val(`cmax_${_ix}`);
    await this.act(`/updateMaxChargeA?value=${encodeURIComponent(v)}`);
  }
  private async applyMaxDischarge(_ix: number) {
    const v = this.val(`dmax_${_ix}`);
    await this.act(`/updateMaxDischargeA?value=${encodeURIComponent(v)}`);
  }

  render() {
    if (this.loading) return html`<p>Loading…</p>`;
    if (this.err) return html`<p class="err">${this.err}</p><button type="button" @click=${this.load}>Retry</button>`;
    const d = this.dash;
    if (!d) return html`<p>No data</p>`;

    const paused = d.contactor.emulator_pause_request_on;
    const ch = d.charger_detail;

    return html`
      <div class="card">
        <h2>Dashboard</h2>
        <p>${d.version} · ${d.hardware}</p>
        <p>Uptime: ${d.uptime_human} (${d.uptime_s}s) · CPU ${d.cpu_temp_C.toFixed(0)}°C</p>
        <div class="grid">
          <div class="kv"><div class="k">WiFi</div><div class="v">${d.wifi.state_text}</div></div>
          ${d.wifi.connected
            ? html`
                <div class="kv"><div class="k">RSSI</div><div class="v">${d.wifi.rssi} dBm</div></div>
                <div class="kv"><div class="k">IP</div><div class="v">${d.wifi.ip}</div></div>
              `
            : null}
        </div>
        <p class="tools">
          ${d.ui.web_logging_active || d.ui.sd_logging_active
            ? html`<a href="#debuglog">Debug log</a> · `
            : null}
          <a href="#canlog">CAN log</a>
        </p>
        <button type="button" @click=${this.load}>Refresh</button>
        <a class="btn" href="/update" target="_blank" rel="noopener">Firmware update (OTA)</a>
        <button type="button" class="warn" @click=${() => this.act(`/pause?value=${paused ? "false" : "true"}`)}>
          ${paused ? "Resume" : "Pause"} emulator
        </button>
        <button type="button" class="danger" @click=${() => this.act(`/equipmentStop?value=${d.contactor.equipment_stop_active ? "0" : "1"}`)}>
          ${d.contactor.equipment_stop_active ? "Clear equipment stop" : "Equipment STOP"}
        </button>
        <p class="ok">${this.actionMsg}</p>
      </div>

      ${d.performance
        ? html`
            <div class="card">
              <h3>Performance</h3>
              <div class="grid">
                <div class="kv"><div class="k">Free heap</div><div class="v">${d.performance.free_heap}</div></div>
                <div class="kv"><div class="k">Max alloc</div><div class="v">${d.performance.max_alloc_heap}</div></div>
                <div class="kv"><div class="k">Flash</div><div class="v">${d.performance.flash_mb} MB ${d.performance.flash_mode}</div></div>
                <div class="kv"><div class="k">Core max µs</div><div class="v">${d.performance.core_task_max_us}</div></div>
              </div>
            </div>
          `
        : null}

      <div class="card">
        <h3>Components</h3>
        <pre style="margin:0;white-space:pre-wrap;font-size:13px">${JSON.stringify(d.components, null, 2)}</pre>
      </div>

      <div class="card">
        <h3>Contactor / pause</h3>
        <div class="grid">
          <div class="kv"><div class="k">Pause</div><div class="v">${d.contactor.pause_status_text}</div></div>
          <div class="kv"><div class="k">Emulator</div><div class="v">${d.ui.emulator_status}</div></div>
          <div class="kv"><div class="k">Equipment stop</div><div class="v">${d.contactor.equipment_stop_active ? "yes" : "no"}</div></div>
          <div class="kv"><div class="k">Precharge</div><div class="v">${d.contactor.precharge_status}</div></div>
          <div class="kv"><div class="k">Contactors</div><div class="v">${d.contactor.contactors_engaged ? "engaged" : "open"}</div></div>
        </div>
      </div>

      ${ch
        ? html`
            <div class="card">
              <h3>Charger</h3>
              <div class="grid">
                <div class="kv"><div class="k">HV out</div><div class="v">${ch.hvdc_V.toFixed(1)} V · ${ch.hvdc_A.toFixed(2)} A</div></div>
                <div class="kv"><div class="k">LV out</div><div class="v">${ch.lvdc_V.toFixed(1)} V · ${ch.lvdc_A.toFixed(2)} A</div></div>
                <div class="kv"><div class="k">AC in</div><div class="v">${ch.ac_V.toFixed(1)} V · ${ch.ac_A.toFixed(2)} A</div></div>
                <div class="kv"><div class="k">Power</div><div class="v">${ch.output_power_W} W</div></div>
                ${ch.efficiency_supported
                  ? html`<div class="kv"><div class="k">Efficiency</div><div class="v">${ch.efficiency_pct?.toFixed(1)} %</div></div>`
                  : null}
              </div>
            </div>
          `
        : null}

      ${d.packs.map((p, i) => this.renderPack(i, p))}
    `;
  }
}
