import "@conectate/components/ct-button";
import "@conectate/components/ct-card";
import "@conectate/components/ct-icon";
import "@conectate/components/ct-spinner";
import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { styleMap } from "lit/directives/style-map.js";
import { api } from "../api/index.js";
import refreshSvg from "../icons/refresh.svg";
import { sharedStyles } from "../styles/shared-styles.js";
import type { BatteryCellGroup, BatteryPackStatus } from "../types/battery.types.js";
import "./cellmonitor/tesla-3y-pack.js";
import { isTeslaModel3Y } from "./cellmonitor/tesla-3y-layout.js";

@customElement("app-cellmonitor")
export class AppCellmonitor extends LitElement {
	@state() private packs: [BatteryCellGroup, BatteryCellGroup, BatteryCellGroup] | null = null;
	@state() private status: BatteryPackStatus | null = null;
	@state() private battType = 0;
	@state() private err = "";
	private timer: ReturnType<typeof setInterval> | null = null;

	static styles = [
		sharedStyles,
		css`
			.pack-header {
				display: flex;
				align-items: center;
				gap: 10px;
				margin-bottom: 10px;
			}
			.pack-header h3 {
				margin: 0;
			}
			.stats {
				display: flex;
				flex-wrap: wrap;
				gap: 6px;
				margin-bottom: 12px;
			}
			.cells {
				display: flex;
				flex-wrap: wrap;
				gap: 5px;
			}
			.cell {
				position: relative;
				width: 42px;
				height: 56px;
				border-radius: 6px;

				background: var(--color-background);
				overflow: hidden;
				display: flex;
				align-items: flex-end;
				justify-content: center;
			}
			.cell .fill {
				position: absolute;
				inset: auto 0 0 0;
				background: var(--color-primary);
				opacity: 0.55;
				transition: height 300ms ease;
			}
			.cell .mv {
				position: relative;
				font-size: 9.5px;
				font-weight: 650;
				color: var(--high-emphasis);
				padding-bottom: 3px;
				font-family: var(--font-mono);
			}
			.cell.max {
				border-color: var(--color-success);
			}
			.cell.min {
				border-color: var(--color-warning);
			}
			.cell.low {
				border-color: var(--color-error);
			}
			.cell.low .fill {
				background: var(--color-error);
			}
			.cell.bal {
				box-shadow: 0 0 8px var(--color-warning);
			}
			.legend {
				display: flex;
				flex-wrap: wrap;
				gap: 14px;
				margin-top: 12px;
				font-size: 0.75rem;
				color: var(--medium-emphasis);
			}
			.legend i {
				display: inline-block;
				width: 10px;
				height: 10px;
				border-radius: 3px;
				margin-right: 5px;
				vertical-align: -1px;
			}
			.legend .l-max i {
				border-color: var(--color-success);
			}
			.legend .l-min i {
				border-color: var(--color-warning);
			}
			.legend .l-low i {
				border-color: var(--color-error);
			}
			.legend .l-bal i {
				box-shadow: 0 0 6px var(--color-warning);
			}
			.tesla-view {
				margin-bottom: 16px;
			}
		`
	];

	connectedCallback(): void {
		super.connectedCallback();
		this.load();
		this.timer = setInterval(() => this.load(), 5000);
	}

	disconnectedCallback(): void {
		if (this.timer) clearInterval(this.timer);
		super.disconnectedCallback();
	}

	async load() {
		const [cellRes, statusRes, settingsRes] = await Promise.all([
			api.battery.cellmonitor(),
			api.battery.status(),
			api.settings.get()
		]);
		const [data, err] = cellRes;
		if (err) {
			this.err = String(err.error_msg || err);
			return;
		}
		this.err = "";
		this.packs = data.packs;
		const [statusData, statusErr] = statusRes;
		if (!statusErr) this.status = statusData.battery;
		const [settingsData, settingsErr] = settingsRes;
		if (!settingsErr) this.battType = settingsData.select?.BATTTYPE ?? 0;
	}

	renderPack(title: string, pack: BatteryCellGroup) {
		if (!pack.present || pack.cells.length === 0) {
			return html`<ct-card>
				<div class="pack-header">
					<h3>${title}</h3>
					<span class="chip">No cells</span>
				</div>
			</ct-card>`;
		}
		const maxV = Math.max(...pack.cells.map((c) => c.voltage_mV));
		const minV = Math.min(...pack.cells.map((c) => c.voltage_mV));
		const span = Math.max(1, maxV - minV);
		return html`
			<ct-card>
				<div class="pack-header">
					<h3>${title}</h3>
					<span class="chip">${pack.number_of_cells} cells</span>
				</div>
				<div class="stats">
					<span class="chip ok">Max ${maxV} mV</span>
					<span class="chip warn">Min ${minV} mV</span>
					<span class="chip">Δ ${maxV - minV} mV</span>
				</div>
				<div class="cells">
					${pack.cells.map((c, i) => {
						const pct = 15 + ((c.voltage_mV - minV) / span) * 85;
						const cls = [c.balancing ? "bal" : "", c.voltage_mV < 3000 ? "low" : "", c.voltage_mV === maxV ? "max" : "", c.voltage_mV === minV ? "min" : ""].join(" ");
						return html`
							<div class="cell ${cls}" title="Cell ${i + 1}: ${c.voltage_mV} mV${c.balancing ? " · balancing" : ""}">
								<div class="fill" style=${styleMap({ height: `${pct}%` })}></div>
								<span class="mv">${(c.voltage_mV / 1000).toFixed(2)}</span>
							</div>
						`;
					})}
				</div>
				<div class="legend">
					<span class="l-max"><i></i>Highest cell</span>
					<span class="l-min"><i></i>Lowest cell</span>
					<span class="l-low"><i></i>Under 3.0 V</span>
					<span class="l-bal"><i></i>Balancing</span>
				</div>
			</ct-card>
		`;
	}

	private get useTeslaView(): boolean {
		const n = this.packs?.[0]?.number_of_cells ?? this.packs?.[0]?.cells.length ?? 0;
		return isTeslaModel3Y(this.battType, n);
	}

	render() {
		if (!this.packs && !this.err) return html`<div class="state"><ct-spinner></ct-spinner><span>Loading cells…</span></div>`;
		if (this.err)
			return html`<div class="state">
				<p class="err">${this.err}</p>
				<ct-button raised @click=${this.load}>Retry</ct-button>
			</div>`;
		if (!this.packs) return html``;

		if (this.useTeslaView) {
			return html`
				<div class="page-header">
					<h2>Cell monitor</h2>
					<div class="spacer"></div>
					<ct-button flat @click=${this.load}>
						<ct-icon slot="prefix" .svg=${refreshSvg}></ct-icon>
						Refresh
					</ct-button>
					<p class="subtitle">Tesla Model 3/Y brick layout · auto-refreshes every 5 seconds.</p>
				</div>
				<div class="tesla-view">
					<tesla-3y-pack .pack=${this.packs[0]} .status=${this.status}></tesla-3y-pack>
				</div>
			`;
		}

		return html`
			<div class="page-header">
				<h2>Cell monitor</h2>
				<div class="spacer"></div>
				<ct-button flat @click=${this.load}>
					<ct-icon slot="prefix" .svg=${refreshSvg}></ct-icon>
					Refresh
				</ct-button>
				<p class="subtitle">Auto-refreshes every 5 seconds.</p>
			</div>
			${this.renderPack("Pack 1", this.packs[0])} ${this.renderPack("Pack 2", this.packs[1])} ${this.renderPack("Pack 3", this.packs[2])}
		`;
	}
}
