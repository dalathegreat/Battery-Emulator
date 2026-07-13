import "@conectate/components/ct-button";
import "@conectate/components/ct-card";
import "@conectate/components/ct-icon";
import "@conectate/components/ct-spinner";
import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { api } from "../api/index.js";
import dangerousSvg from "../icons/dangerous.svg";
import pauseSvg from "../icons/pause.svg";
import playSvg from "../icons/play.svg";
import refreshSvg from "../icons/refresh.svg";
import updateSvg from "../icons/update.svg";
import { sharedStyles } from "../styles/shared-styles.js";
import type { DashboardPack, DashboardResponse } from "../types/dashboard.types.js";
import type { SettingsResponse } from "../types/settings.types.js";

@customElement("app-dashboard")
export class AppDashboard extends LitElement {
	@state() private dash: DashboardResponse | null = null;
	@state() private limits: SettingsResponse["datalayer"] | null = null;
	@state() private err = "";
	@state() private loading = true;
	@state() private actionMsg = "";

	static styles = [
		sharedStyles,
		css`
			.kpis {
				display: grid;
				grid-template-columns: repeat(auto-fit, minmax(170px, 1fr));
				gap: 12px;
				margin-bottom: 16px;
			}
			.kpi {
				background: var(--color-background);

				border-radius: var(--radius-card);
				padding: 14px 16px;
			}
			.kpi .label {
				font-size: 0.72rem;
				color: var(--medium-emphasis);
				text-transform: uppercase;
				letter-spacing: 0.05em;
			}
			.kpi .value {
				font-size: 1.5rem;
				font-weight: 750;
				color: var(--high-emphasis);
				margin: 4px 0 2px;
				letter-spacing: -0.02em;
			}
			.kpi .value small {
				font-size: 0.85rem;
				font-weight: 500;
				color: var(--medium-emphasis);
			}
			.kpi .bar {
				margin-top: 8px;
			}
			.kpi.accent .value {
				color: var(--color-primary);
			}
			.pack-header {
				display: flex;
				align-items: center;
				gap: 10px;
				margin-bottom: 12px;
			}
			.pack-header h3 {
				margin: 0;
			}
			.actions {
				display: flex;
				flex-wrap: wrap;
				gap: 8px;
				margin-top: 12px;
			}
			.limit-form {
				display: grid;
				grid-template-columns: minmax(110px, auto) 100px auto;
				align-items: center;
				gap: 8px 10px;
				font-size: 0.85rem;
				color: var(--medium-emphasis);
				max-width: 420px;
			}
			.meta {
				font-size: 0.82rem;
				color: var(--medium-emphasis);
				margin: 2px 0 0;
			}
		`
	];

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

	private kpi(label: string, value: unknown, unit = "", opts?: { barPct?: number; accent?: boolean }) {
		return html`
			<div class="kpi ${opts?.accent ? "accent" : ""}">
				<div class="label">${label}</div>
				<div class="value">${value}${unit ? html` <small>${unit}</small>` : null}</div>
				${opts?.barPct != null ? html`<div class="bar"><i style="width:${Math.max(0, Math.min(100, opts.barPct))}%"></i></div>` : null}
			</div>
		`;
	}

	private renderPack(ix: number, p: DashboardPack) {
		const label = `Battery ${ix + 1}`;
		if (!p.present) {
			return html`<ct-card>
				<div class="pack-header">
					<h3>${label}</h3>
					<span class="chip">Not present</span>
				</div>
			</ct-card>`;
		}
		return html`
			<ct-card>
				<div class="pack-header">
					<h3>${label}</h3>
					<span class="chip ${p.can_alive ? "ok" : "error"}">CAN ${p.can_alive ? "alive" : "dead"}</span>
					<span class="chip">${p.bms_status}</span>
				</div>
				<div class="kpis">
					${this.kpi("SOC reported", p.soc_reported_pct.toFixed(1), "%", { barPct: p.soc_reported_pct, accent: true })}
					${this.kpi("SOC real", p.soc_real_pct.toFixed(1), "%", { barPct: p.soc_real_pct })} ${this.kpi("Power", p.power_W, "W")}
					${this.kpi("Voltage", p.voltage_V.toFixed(1), "V")} ${this.kpi("Current", p.current_A.toFixed(1), "A")}
				</div>
				<div class="grid">
					<div class="kv">
						<div class="k">Flow</div>
						<div class="v">${p.flow ?? "—"}</div>
					</div>
					<div class="kv">
						<div class="k">SOH</div>
						<div class="v">${p.soh_pct.toFixed(1)} %</div>
					</div>
					<div class="kv">
						<div class="k">Temp</div>
						<div class="v">${p.temp_min_C.toFixed(1)} – ${p.temp_max_C.toFixed(1)} °C</div>
					</div>
					<div class="kv">
						<div class="k">Cells Δ</div>
						<div class="v">${p.cell_delta_mV} mV</div>
					</div>
					<div class="kv">
						<div class="k">Balancing</div>
						<div class="v">${p.balancing_status} (${p.balancing_active_cells})</div>
					</div>
					${p.reported_total_Wh != null
						? html`<div class="kv">
								<div class="k">Reported total</div>
								<div class="v">${p.reported_total_Wh} Wh</div>
							</div>`
						: null}
					${p.max_charge_current_A != null
						? html`<div class="kv">
								<div class="k">Max chg / dchg</div>
								<div class="v">${p.max_charge_current_A.toFixed(1)} / ${p.max_discharge_current_A?.toFixed(1) ?? "—"} A</div>
							</div>`
						: null}
				</div>
				${ix === 0 && this.limits
					? html`
							<h4>Live limits (pack 1)</h4>
							<div class="limit-form">
								<span>Total Wh</span>
								<input type="number" id="wh_${ix}" .value=${String(p.total_Wh)} />
								<ct-button raised @click=${() => this.applyWh(ix)}>Apply</ct-button>

								<span>SOC max %</span>
								<input type="number" id="smax_${ix}" step="0.1" .value=${String(this.limits.soc_max_pct)} />
								<ct-button raised @click=${() => this.applySocMax(ix)}>Apply</ct-button>

								<span>SOC min %</span>
								<input type="number" id="smin_${ix}" step="0.1" .value=${String(this.limits.soc_min_pct)} />
								<ct-button raised @click=${() => this.applySocMin(ix)}>Apply</ct-button>

								<span>Max charge A</span>
								<input type="number" id="cmax_${ix}" step="0.1" .value=${String(this.limits.max_charge_A)} />
								<ct-button raised @click=${() => this.applyMaxCharge(ix)}>Apply</ct-button>

								<span>Max discharge A</span>
								<input type="number" id="dmax_${ix}" step="0.1" .value=${String(this.limits.max_discharge_A)} />
								<ct-button raised @click=${() => this.applyMaxDischarge(ix)}>Apply</ct-button>

								<span>Scaled SOC</span>
								<span class="chip ${p.soc_scaling_active ? "ok" : ""}">${p.soc_scaling_active ? "on" : "off"}</span>
								<ct-button raised @click=${() => this.act(`/updateUseScaledSOC?value=${p.soc_scaling_active ? 0 : 1}`)}>Toggle</ct-button>
							</div>
						`
					: null}
			</ct-card>
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
		if (this.loading) return html`<div class="state"><ct-spinner></ct-spinner><span>Loading dashboard…</span></div>`;
		if (this.err)
			return html`<div class="state">
				<p class="err">${this.err}</p>
				<ct-button raised @click=${this.load}>Retry</ct-button>
			</div>`;
		const d = this.dash;
		if (!d) return html`<div class="state"><span class="empty">No data</span></div>`;

		const paused = d.contactor.emulator_pause_request_on;
		const ch = d.charger_detail;

		return html`
			<div class="page-header">
				<h2>Dashboard</h2>
				<span class="chip">${d.version}</span>
				<span class="chip">${d.hardware}</span>
				<div class="spacer"></div>
				<p class="subtitle">Uptime ${d.uptime_human} · CPU ${d.cpu_temp_C.toFixed(0)} °C</p>
			</div>

			<ct-card>
				<h3>System</h3>
				<div class="grid">
					<div class="kv">
						<div class="k">WiFi</div>
						<div class="v">${d.wifi.state_text}</div>
					</div>
					${d.wifi.connected
						? html`
								<div class="kv">
									<div class="k">RSSI</div>
									<div class="v">${d.wifi.rssi} dBm</div>
								</div>
								<div class="kv">
									<div class="k">IP</div>
									<div class="v">${d.wifi.ip}</div>
								</div>
							`
						: null}
					<div class="kv">
						<div class="k">Emulator</div>
						<div class="v">${d.ui.emulator_status}</div>
					</div>
					<div class="kv">
						<div class="k">Uptime</div>
						<div class="v">${d.uptime_s}s</div>
					</div>
				</div>
				<div class="actions">
					<ct-button flat gap @click=${this.load}>
						<ct-icon slot="prefix" .svg=${refreshSvg}></ct-icon>
						Refresh
					</ct-button>
					<a href="/update" target="_blank" rel="noopener" style="text-decoration:none">
						<ct-button light gap>
							<ct-icon slot="prefix" .svg=${updateSvg}></ct-icon>
							Firmware update (OTA)
						</ct-button>
					</a>
					<ct-button light style="--color-primary: var(--color-warning)" gap @click=${() => this.act(`/pause?value=${paused ? "false" : "true"}`)}>
						<ct-icon slot="prefix" .svg=${paused ? playSvg : pauseSvg}></ct-icon>
						${paused ? "Resume" : "Pause"} emulator
					</ct-button>
					<ct-button light style="--color-primary: var(--color-error)" gap @click=${() => this.act(`/equipmentStop?value=${d.contactor.equipment_stop_active ? "0" : "1"}`)}>
						<ct-icon slot="prefix" .svg=${dangerousSvg}></ct-icon>
						${d.contactor.equipment_stop_active ? "Clear equipment stop" : "Equipment STOP"}
					</ct-button>
				</div>
				<p class="meta">
					${d.ui.web_logging_active || d.ui.sd_logging_active ? html`<a href="/debuglog">Debug log</a> · ` : null}
					<a href="/canlog">CAN log</a>
				</p>
				${this.actionMsg ? html`<p class="ok">${this.actionMsg}</p>` : null}
			</ct-card>

			${d.performance
				? html`
						<ct-card>
							<h3>Performance</h3>
							<div class="grid">
								<div class="kv">
									<div class="k">Free heap</div>
									<div class="v">${d.performance.free_heap}</div>
								</div>
								<div class="kv">
									<div class="k">Max alloc</div>
									<div class="v">${d.performance.max_alloc_heap}</div>
								</div>
								<div class="kv">
									<div class="k">Flash</div>
									<div class="v">${d.performance.flash_mb} MB ${d.performance.flash_mode}</div>
								</div>
								<div class="kv">
									<div class="k">Core max µs</div>
									<div class="v">${d.performance.core_task_max_us}</div>
								</div>
							</div>
						</ct-card>
					`
				: null}

			<ct-card>
				<h3>Components</h3>
				<div class="grid">
					${Object.entries(d.components).map(
						([k, v]) =>
							html`<div class="kv">
								<div class="k">${k}</div>
								<div class="v">${typeof v === "boolean" ? (v ? "yes" : "no") : (v ?? "—")}</div>
							</div>`
					)}
				</div>
			</ct-card>

			<ct-card>
				<h3>Contactor / pause</h3>
				<div class="grid">
					<div class="kv">
						<div class="k">Pause</div>
						<div class="v">${d.contactor.pause_status_text}</div>
					</div>
					<div class="kv">
						<div class="k">Emulator</div>
						<div class="v">${d.ui.emulator_status}</div>
					</div>
					<div class="kv">
						<div class="k">Equipment stop</div>
						<div class="v">${d.contactor.equipment_stop_active ? "yes" : "no"}</div>
					</div>
					<div class="kv">
						<div class="k">Precharge</div>
						<div class="v">${d.contactor.precharge_status}</div>
					</div>
					<div class="kv">
						<div class="k">Contactors</div>
						<div class="v">${d.contactor.contactors_engaged ? "engaged" : "open"}</div>
					</div>
				</div>
			</ct-card>

			${ch
				? html`
						<ct-card>
							<h3>Charger</h3>
							<div class="grid">
								<div class="kv">
									<div class="k">HV out</div>
									<div class="v">${ch.hvdc_V.toFixed(1)} V · ${ch.hvdc_A.toFixed(2)} A</div>
								</div>
								<div class="kv">
									<div class="k">LV out</div>
									<div class="v">${ch.lvdc_V.toFixed(1)} V · ${ch.lvdc_A.toFixed(2)} A</div>
								</div>
								<div class="kv">
									<div class="k">AC in</div>
									<div class="v">${ch.ac_V.toFixed(1)} V · ${ch.ac_A.toFixed(2)} A</div>
								</div>
								<div class="kv">
									<div class="k">Power</div>
									<div class="v">${ch.output_power_W} W</div>
								</div>
								${ch.efficiency_supported
									? html`<div class="kv">
											<div class="k">Efficiency</div>
											<div class="v">${ch.efficiency_pct?.toFixed(1)} %</div>
										</div>`
									: null}
							</div>
						</ct-card>
					`
				: null}
			${d.packs.map((p, i) => this.renderPack(i, p))}
		`;
	}
}
