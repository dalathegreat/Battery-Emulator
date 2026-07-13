import { LitElement, css, html, nothing } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { repeat } from "lit/directives/repeat.js";
import { styleMap } from "lit/directives/style-map.js";
import packImg from "../../../res/batteries/tesla-model-3.webp";
import type { BatteryCellGroup, CellEntry } from "../../types/battery.types.js";
import type { BatteryPackStatus } from "../../types/battery.types.js";
import {
	TESLA_3Y_MODULES,
	modeVoltage_mV,
	moduleBrickCount,
	moduleIdForBrick,
	splitIntoModules
} from "./tesla-3y-layout.js";

@customElement("tesla-3y-pack")
export class Tesla3yPack extends LitElement {
	@property({ attribute: false }) pack: BatteryCellGroup | null = null;
	@property({ attribute: false }) status: BatteryPackStatus | null = null;

	@state() private showCac = true;
	/** Sticky selection from click (−1 = none). */
	@state() private selectedIndex = -1;
	/** Ephemeral hover (−1 = none). */
	@state() private hoverIndex = -1;

	static styles = css`
		:host {
			display: block;
		}

		.panel {
			background: #0a0e10;
			border-radius: var(--radius-card, 14px);
			overflow: hidden;
			border: 1px solid var(--color-outline, #8ba3ad2e);
		}

		.stats {
			display: flex;
			flex-wrap: wrap;
			align-items: center;
			gap: 6px 14px;
			padding: 10px 14px;
			font-size: 0.78rem;
			color: #e8eef2;
			font-family: var(--font-mono, ui-monospace, monospace);
			border-bottom: 1px solid #ffffff12;
		}

		.stats .title {
			font-family: var(--font-body, system-ui, sans-serif);
			font-weight: 700;
			letter-spacing: 0.04em;
			margin-right: 4px;
			white-space: nowrap;
		}

		.stats .stat {
			white-space: nowrap;
			color: #c5d0d8;
		}

		.stats .stat b {
			color: #fff;
			font-weight: 600;
		}

		.stats .spacer {
			flex: 1;
			min-width: 8px;
		}

		.toggle {
			display: inline-flex;
			align-items: center;
			gap: 8px;
			cursor: pointer;
			user-select: none;
			font-family: var(--font-body, system-ui, sans-serif);
			font-size: 0.8rem;
			color: #dbe4e9;
		}

		.toggle input {
			appearance: none;
			width: 36px;
			height: 20px;
			border-radius: 999px;
			background: #3a4a52;
			position: relative;
			cursor: pointer;
			transition: background 160ms ease;
			margin: 0;
		}

		.toggle input::after {
			content: "";
			position: absolute;
			top: 2px;
			left: 2px;
			width: 16px;
			height: 16px;
			border-radius: 50%;
			background: #fff;
			transition: transform 160ms ease;
		}

		.toggle input:checked {
			background: #3b82f6;
		}

		.toggle input:checked::after {
			transform: translateX(16px);
		}

		.stats .stat.min b {
			color: #f87171;
		}
		.stats .stat.max b {
			color: #fbbf24;
		}

		.pack-stage {
			position: relative;
			width: 100%;
			aspect-ratio: 720 / 499;
			background: #000;
			overflow: hidden;
		}

		.pack-bg {
			position: absolute;
			inset: 0;
			width: 100%;
			height: 100%;
			object-fit: contain;
			object-position: center;
			opacity: 0.95;
			filter: grayscale(0.35) contrast(1.08) brightness(0.92);
			pointer-events: none;
			user-select: none;
		}

		.modules {
			position: absolute;
			inset: 0;
			z-index: 1;
			pointer-events: none;
		}

		.modules .module {
			pointer-events: auto;
		}

		/*
		 * Module slots keyed to the 4 horizontal pack lanes (left of penthouse).
		 * Coordinates measured against tesla-model-3.webp (720×499).
		 */
		.module {
			position: absolute;
			left: 14.5%;
			width: 56%;
			height: 17.5%;
			background: rgb(6 10 14 / 0.78);
			border-radius: 7px;
			padding: 3px 5px 4px;
			display: flex;
			flex-direction: column;
			min-height: 0;
			border: 1px solid #ffffff14;
			box-sizing: border-box;
		}
		.module.m1 {
			top: 8.5%;
		}
		.module.m2 {
			top: 31%;
		}
		.module.m3 {
			top: 52%;
		}
		.module.m4 {
			top: 74%;
		}

		.module-label {
			font-size: 0.58rem;
			font-weight: 650;
			color: #9eb0ba;
			letter-spacing: 0.03em;
			margin: 0 0 2px 1px;
			line-height: 1;
			flex: 0 0 auto;
		}

		.brick-rows {
			flex: 1;
			display: flex;
			flex-direction: column;
			justify-content: space-evenly;
			gap: 2px;
			min-height: 0;
		}

		.brick-row {
			display: flex;
			gap: 2px;
			min-height: 0;
			flex: 1 1 0;
		}

		.brick {
			flex: 1 1 0;
			min-width: 0;
			max-width: calc((100% - 26px) / 14);
			border-radius: 3px;
			background: #1a5c34;
			color: #f4faf6;
			display: flex;
			flex-direction: column;
			justify-content: space-between;
			padding: 1px 2px 2px;
			font-family: var(--font-mono, ui-monospace, monospace);
			line-height: 1.05;
			border: 1.5px solid transparent;
			box-sizing: border-box;
			overflow: hidden;
			cursor: pointer;
		}

		.brick .top {
			display: flex;
			align-items: center;
			justify-content: space-between;
			gap: 1px;
			font-size: clamp(5px, 0.7vw, 8px);
			opacity: 0.92;
		}

		.brick .volts {
			font-size: clamp(5.5px, 0.8vw, 9px);
			font-weight: 700;
			letter-spacing: -0.03em;
		}

		.brick .cac {
			font-size: clamp(5px, 0.68vw, 7.5px);
			opacity: 0.88;
		}

		.brick .mark {
			width: 0.55em;
			height: 0.55em;
			border-radius: 50%;
			flex-shrink: 0;
			background: #38bdf8;
		}

		.brick .mark.up {
			clip-path: polygon(50% 0, 100% 100%, 0 100%);
			background: #60a5fa;
			border-radius: 0;
		}

		.brick .mark.down {
			clip-path: polygon(0 0, 100% 0, 50% 100%);
			background: #60a5fa;
			border-radius: 0;
		}

		.brick.min {
			background: #dc2626;
			border-color: #fca5a5;
		}

		.brick.max {
			border-color: #f59e0b;
			box-shadow: inset 0 0 0 1px #f59e0b88;
		}

		.brick.bal {
			border-color: #38bdf8;
			box-shadow: 0 0 0 1px #38bdf888;
		}

		.brick.low:not(.min) {
			background: #b45309;
		}

		.brick.selected {
			outline: 2px solid #38bdf8;
			outline-offset: 1px;
			z-index: 2;
		}

		.brick.empty {
			visibility: hidden;
			pointer-events: none;
			cursor: default;
		}

		@media (max-width: 900px) {
			.stats {
				font-size: 0.7rem;
				gap: 4px 10px;
			}
			.module {
				padding: 2px 3px 3px;
				height: 18%;
			}
			.module-label {
				font-size: 0.5rem;
			}
		}

		/* —— Voltage bar chart (pure divs) —— */
		.chart {
			border-top: 1px solid #ffffff12;
			padding: 12px 14px 14px;
		}

		.chart-head {
			display: flex;
			flex-wrap: wrap;
			align-items: center;
			gap: 10px 16px;
			margin-bottom: 10px;
		}

		.chart-head h3 {
			margin: 0;
			font-size: 0.82rem;
			font-weight: 650;
			color: #e8eef2;
			letter-spacing: 0.03em;
		}

		.chart-legend {
			display: flex;
			flex-wrap: wrap;
			gap: 12px;
			font-size: 0.72rem;
			color: #9eb0ba;
		}

		.chart-legend span {
			display: inline-flex;
			align-items: center;
			gap: 5px;
		}

		.chart-legend i {
			display: inline-block;
			width: 9px;
			height: 9px;
			border-radius: 2px;
			background: #1a5c34;
		}

		.chart-legend .l-min i {
			background: #dc2626;
		}
		.chart-legend .l-max i {
			background: #1a5c34;
			box-shadow: inset 0 0 0 1.5px #f59e0b;
		}
		.chart-legend .l-bal i {
			background: #1a5c34;
			box-shadow: inset 0 0 0 1.5px #38bdf8;
		}

		.chart-body {
			display: grid;
			grid-template-columns: 52px 1fr;
			gap: 8px;
			align-items: stretch;
			min-height: 160px;
		}

		.y-axis {
			display: flex;
			flex-direction: column;
			justify-content: space-between;
			align-items: flex-end;
			padding: 4px 0 22px;
			font-family: var(--font-mono, ui-monospace, monospace);
			font-size: 0.65rem;
			color: #8ba3ad;
			line-height: 1;
		}

		.bars-wrap {
			position: relative;
			min-width: 0;
			display: flex;
			flex-direction: column;
		}

		.bars {
			flex: 1;
			display: flex;
			align-items: stretch;
			gap: 1px;
			min-height: 140px;
			padding: 4px 2px 0;
			background:
				linear-gradient(to top, #ffffff08 1px, transparent 1px) 0 0 / 100% 25%,
				#0d1418;
			border: 1px solid #ffffff10;
			border-radius: 8px 8px 0 0;
		}

		.bar-col {
			flex: 1 1 0;
			min-width: 0;
			height: 100%;
			display: flex;
			flex-direction: column;
			justify-content: flex-end;
			align-items: center;
			position: relative;
			cursor: pointer;
			outline: none;
		}

		.bar-col:hover .bar,
		.bar-col.hover .bar {
			filter: brightness(1.25);
		}

		.bar-col.selected .bar {
			filter: brightness(1.35);
			box-shadow: 0 0 0 2px #38bdf8, 0 0 10px #38bdf866;
		}

		.bar-col.mod-start::before {
			content: "";
			position: absolute;
			left: -1px;
			top: 0;
			bottom: 0;
			width: 1px;
			background: #ffffff28;
			z-index: 1;
		}

		.bar {
			width: 100%;
			max-width: 14px;
			min-height: 2px;
			border-radius: 2px 2px 0 0;
			background: #22c55e;
			transition: height 280ms ease, filter 120ms ease, box-shadow 120ms ease;
		}

		.bar-col.min .bar {
			background: #ef4444;
		}

		.bar-col.max .bar {
			background: #22c55e;
			box-shadow: inset 0 0 0 1.5px #f59e0b;
		}

		.bar-col.max.selected .bar,
		.bar-col.bal.selected .bar {
			box-shadow: 0 0 0 2px #38bdf8, inset 0 0 0 1.5px #f59e0b;
		}

		.bar-col.bal .bar {
			box-shadow: inset 0 0 0 1.5px #38bdf8;
		}

		.bar-col.low:not(.min) .bar {
			background: #f59e0b;
		}

		.tip {
			position: absolute;
			top: 8px;
			z-index: 5;
			min-width: 168px;
			max-width: 220px;
			padding: 8px 10px;
			border-radius: 8px;
			background: #152028f2;
			border: 1px solid #3b82f688;
			box-shadow: 0 8px 24px #00000088;
			color: #e8eef2;
			font-family: var(--font-body, system-ui, sans-serif);
			font-size: 0.75rem;
			line-height: 1.35;
			pointer-events: none;
			transform: translateX(-50%);
		}

		.tip::after {
			content: "";
			position: absolute;
			top: 100%;
			left: 50%;
			transform: translateX(-50%);
			border: 6px solid transparent;
			border-top-color: #152028f2;
		}

		.tip .tip-title {
			font-weight: 700;
			font-size: 0.8rem;
			margin-bottom: 4px;
			color: #fff;
		}

		.tip .tip-row {
			display: flex;
			justify-content: space-between;
			gap: 12px;
			font-family: var(--font-mono, ui-monospace, monospace);
			font-size: 0.7rem;
			color: #b6c4cc;
		}

		.tip .tip-row b {
			color: #f2f7f9;
			font-weight: 600;
		}

		.tip .tip-flags {
			display: flex;
			flex-wrap: wrap;
			gap: 4px;
			margin-top: 6px;
		}

		.tip .flag {
			font-size: 0.62rem;
			font-weight: 650;
			padding: 1px 6px;
			border-radius: 999px;
			background: #ffffff14;
			color: #c5d0d8;
		}

		.tip .flag.min {
			background: #dc262633;
			color: #fca5a5;
		}
		.tip .flag.max {
			background: #f59e0b33;
			color: #fcd34d;
		}
		.tip .flag.bal {
			background: #38bdf833;
			color: #7dd3fc;
		}

		.x-axis {
			display: flex;
			gap: 1px;
			padding: 4px 2px 0;
			font-family: var(--font-mono, ui-monospace, monospace);
			font-size: 0.58rem;
			color: #6b7f8a;
			min-height: 18px;
		}

		.x-axis .tick {
			flex: 1 1 0;
			min-width: 0;
			text-align: center;
			overflow: hidden;
		}

		.x-axis .tick.mod-start {
			color: #9eb0ba;
			font-weight: 600;
		}
	`;

	private onToggleCac(e: Event) {
		const el = e.target as HTMLInputElement;
		this.showCac = el.checked;
	}

	private get activeIndex(): number {
		return this.hoverIndex >= 0 ? this.hoverIndex : this.selectedIndex;
	}

	private selectBrick(index: number) {
		this.selectedIndex = this.selectedIndex === index ? -1 : index;
	}

	private onBarKeydown(e: KeyboardEvent, index: number) {
		if (e.key === "Enter" || e.key === " ") {
			e.preventDefault();
			this.selectBrick(index);
		}
	}

	private renderTip(cells: CellEntry[], minV: number, maxV: number) {
		const i = this.activeIndex;
		if (i < 0 || i >= cells.length) return nothing;
		const cell = cells[i];
		const mod = moduleIdForBrick(i);
		const n = cells.length;
		const leftPct = ((i + 0.5) / n) * 100;
		// Keep tooltip inside the chart: clamp near edges
		const clamped = Math.min(92, Math.max(8, leftPct));
		const isMin = cell.voltage_mV === minV && minV !== maxV;
		const isMax = cell.voltage_mV === maxV && minV !== maxV;
		const delta = cell.voltage_mV - minV;

		return html`
			<div
				class="tip"
				style=${styleMap({ left: `${clamped}%` })}
				role="tooltip"
			>
				<div class="tip-title">Brick #${i + 1}</div>
				<div class="tip-row"><span>Module</span><b>${mod || "—"}</b></div>
				<div class="tip-row"><span>Voltage</span><b>${(cell.voltage_mV / 1000).toFixed(4)} V</b></div>
				<div class="tip-row"><span>Raw</span><b>${cell.voltage_mV} mV</b></div>
				<div class="tip-row"><span>Δ vs min</span><b>+${delta} mV</b></div>
				${cell.capacity_Ah != null
					? html`<div class="tip-row"><span>CAC</span><b>${cell.capacity_Ah.toFixed(1)} Ah</b></div>`
					: nothing}
				${isMin || isMax || cell.balancing
					? html`<div class="tip-flags">
							${isMin ? html`<span class="flag min">MIN</span>` : nothing}
							${isMax ? html`<span class="flag max">MAX</span>` : nothing}
							${cell.balancing ? html`<span class="flag bal">BALANCING</span>` : nothing}
						</div>`
					: nothing}
			</div>
		`;
	}

	private renderBrick(
		cell: CellEntry | undefined,
		index: number,
		minV: number,
		maxV: number,
		hasCac: boolean
	) {
		if (!cell) {
			return html`<div class="brick empty" aria-hidden="true"></div>`;
		}
		const isMin = cell.voltage_mV === minV && minV !== maxV;
		const isMax = cell.voltage_mV === maxV && minV !== maxV;
		const isLow = cell.voltage_mV < 3000;
		const cls = {
			brick: true,
			min: isMin,
			max: isMax,
			bal: cell.balancing,
			low: isLow,
			selected: this.selectedIndex === index || this.hoverIndex === index
		};
		const mark =
			isMin
				? html`<span class="mark down" title="Min"></span>`
				: isMax
					? html`<span class="mark up" title="Max"></span>`
					: cell.balancing
						? html`<span class="mark" title="Balancing"></span>`
						: nothing;

		const cac =
			this.showCac && hasCac && cell.capacity_Ah != null
				? html`<div class="cac">${cell.capacity_Ah.toFixed(1)}Ah</div>`
				: this.showCac && hasCac
					? html`<div class="cac">—</div>`
					: nothing;

		return html`
			<div
				class=${classMap(cls)}
				role="button"
				tabindex="0"
				aria-pressed=${this.selectedIndex === index ? "true" : "false"}
				aria-label=${`Brick ${index + 1}, ${(cell.voltage_mV / 1000).toFixed(4)} volts`}
				@click=${() => this.selectBrick(index)}
				@keydown=${(e: KeyboardEvent) => this.onBarKeydown(e, index)}
				@mouseenter=${() => {
					this.hoverIndex = index;
				}}
				@mouseleave=${() => {
					if (this.hoverIndex === index) this.hoverIndex = -1;
				}}
			>
				<div class="top">
					<span>#${index + 1}</span>
					${mark}
				</div>
				<div class="volts">${(cell.voltage_mV / 1000).toFixed(4)}</div>
				${cac}
			</div>
		`;
	}

	private renderModule(
		modIndex: number,
		cells: (CellEntry | undefined)[],
		startIndex: number,
		minV: number,
		maxV: number,
		hasCac: boolean
	) {
		const mod = TESLA_3Y_MODULES[modIndex];
		let offset = 0;
		return html`
			<section class="module m${mod.id}" aria-label=${mod.label}>
				<div class="module-label">${mod.label}</div>
				<div class="brick-rows">
					${mod.rows.map((count) => {
						const rowCells = Array.from({ length: count }, (_, i) => cells[offset + i]);
						const rowStart = startIndex + offset;
						offset += count;
						return html`
							<div class="brick-row">
								${repeat(
									rowCells,
									(_, i) => rowStart + i,
									(cell, i) => this.renderBrick(cell, rowStart + i, minV, maxV, hasCac)
								)}
							</div>
						`;
					})}
				</div>
			</section>
		`;
	}

	/** Module start indices for divider ticks on the bar chart. */
	private moduleStarts(): number[] {
		const starts: number[] = [];
		let acc = 0;
		for (const mod of TESLA_3Y_MODULES) {
			starts.push(acc);
			acc += moduleBrickCount(mod.rows);
		}
		return starts;
	}

	private renderBarChart(cells: CellEntry[], minV: number, maxV: number) {
		const pad = Math.max(20, Math.round((maxV - minV) * 0.15));
		const lo = minV - pad;
		const hi = Math.max(maxV + pad, lo + 50);
		const span = hi - lo;
		const mid = Math.round((lo + hi) / 2);
		const starts = new Set(this.moduleStarts());

		return html`
			<section class="chart" aria-label="Brick voltage chart">
				<div class="chart-head">
					<h3>VOLTAGE BARS</h3>
					<div class="chart-legend">
						<span><i></i>Normal</span>
						<span class="l-min"><i></i>Min</span>
						<span class="l-max"><i></i>Max</span>
						<span class="l-bal"><i></i>Balancing</span>
					</div>
				</div>
				<div class="chart-body">
					<div class="y-axis" aria-hidden="true">
						<span>${(hi / 1000).toFixed(3)}</span>
						<span>${(mid / 1000).toFixed(3)}</span>
						<span>${(lo / 1000).toFixed(3)}</span>
					</div>
					<div class="bars-wrap">
						${this.renderTip(cells, minV, maxV)}
						<div
							class="bars"
							role="listbox"
							aria-label="Brick voltages"
							@mouseleave=${() => {
								this.hoverIndex = -1;
							}}
						>
							${repeat(
								cells,
								(_, i) => i,
								(cell, i) => {
									const pct = Math.max(4, ((cell.voltage_mV - lo) / span) * 100);
									const cls = {
										"bar-col": true,
										min: cell.voltage_mV === minV && minV !== maxV,
										max: cell.voltage_mV === maxV && minV !== maxV,
										bal: cell.balancing,
										low: cell.voltage_mV < 3000,
										"mod-start": starts.has(i) && i > 0,
										selected: this.selectedIndex === i,
										hover: this.hoverIndex === i
									};
									return html`
										<div
											class=${classMap(cls)}
											role="option"
											tabindex="0"
											aria-selected=${this.selectedIndex === i ? "true" : "false"}
											aria-label=${`Brick ${i + 1}, ${(cell.voltage_mV / 1000).toFixed(4)} volts`}
											@mouseenter=${() => {
												this.hoverIndex = i;
											}}
											@click=${() => this.selectBrick(i)}
											@keydown=${(e: KeyboardEvent) => this.onBarKeydown(e, i)}
										>
											<div class="bar" style=${styleMap({ height: `${pct}%` })}></div>
										</div>
									`;
								}
							)}
						</div>
						<div class="x-axis" aria-hidden="true">
							${repeat(
								cells,
								(_, i) => `t${i}`,
								(_, i) => {
									const show = i === 0 || starts.has(i) || i === cells.length - 1 || (i + 1) % 8 === 0;
									return html`<span class="tick ${starts.has(i) ? "mod-start" : ""}"
										>${show ? i + 1 : ""}</span
									>`;
								}
							)}
						</div>
					</div>
				</div>
			</section>
		`;
	}

	render() {
		const pack = this.pack;
		if (!pack?.present || pack.cells.length === 0) {
			return html`<div class="panel"><div class="stats"><span class="title">Brick data</span><span class="stat">No cells</span></div></div>`;
		}

		const voltages = pack.cells.map((c) => c.voltage_mV);
		const minV = Math.min(...voltages);
		const maxV = Math.max(...voltages);
		const modeV = modeVoltage_mV(voltages) ?? minV;
		const variance = maxV - minV;
		const hasCac = pack.cells.some((c) => c.capacity_Ah != null);

		const packV =
			this.status?.voltage_V ??
			pack.cells.reduce((s, c) => s + c.voltage_mV, 0) / 1000;
		const tMin = this.status?.temp_min_C;
		const tMax = this.status?.temp_max_C;
		const tMean =
			tMin != null && tMax != null ? (tMin + tMax) / 2 : undefined;

		const modules = splitIntoModules(pack.cells);
		const starts: number[] = [];
		let acc = 0;
		for (const mod of TESLA_3Y_MODULES) {
			starts.push(acc);
			acc += moduleBrickCount(mod.rows);
		}

		return html`
			<div class="panel">
				<div class="stats">
					<span class="title">BRICK DATA</span>
					<span class="stat min">Minimum: <b>${(minV / 1000).toFixed(4)}V</b></span>
					<span class="stat max">Maximum: <b>${(maxV / 1000).toFixed(4)}V</b></span>
					<span class="stat">Mode: <b>${(modeV / 1000).toFixed(3)}V</b></span>
					<span class="stat">Variance: <b>${variance.toFixed(2)} mV</b></span>
					<span class="stat">Full Pack: <b>${packV.toFixed(4)}V</b></span>
					${tMin != null
						? html`<span class="stat">Cell Temp Min: <b>${tMin.toFixed(1)}°C</b></span>`
						: nothing}
					${tMax != null
						? html`<span class="stat">Cell Temp Max: <b>${tMax.toFixed(1)}°C</b></span>`
						: nothing}
					${tMean != null
						? html`<span class="stat">Cell Temp Mean: <b>${tMean.toFixed(1)}°C</b></span>`
						: nothing}
					<span class="spacer"></span>
					${hasCac
						? html`<label class="toggle">
								<input type="checkbox" .checked=${this.showCac} @change=${this.onToggleCac} />
								Show CAC
							</label>`
						: nothing}
				</div>
				<div class="pack-stage">
					<img class="pack-bg" src=${packImg} alt="Tesla Model 3/Y battery pack" draggable="false" />
					<div class="modules">
						${TESLA_3Y_MODULES.map((_, i) =>
							this.renderModule(i, modules[i] ?? [], starts[i], minV, maxV, hasCac)
						)}
					</div>
				</div>
				${this.renderBarChart(pack.cells, minV, maxV)}
			</div>
		`;
	}
}

declare global {
	interface HTMLElementTagNameMap {
		"tesla-3y-pack": Tesla3yPack;
	}
}
