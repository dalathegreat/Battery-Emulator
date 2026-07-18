import "@conectate/components/ct-button";
import "@conectate/components/ct-card";
import "@conectate/components/ct-icon";
import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { api } from "../api/index.js";
import refreshSvg from "../icons/refresh.svg";
import { sharedStyles } from "../styles/shared-styles.js";
import type { BatteryExtraEntry, BatteryExtraRow } from "../types/battery-extra.types.js";
import type { BatteryCommandDef } from "../types/battery.types.js";

function groupExtraRows(rows: BatteryExtraRow[]) {
	const sections: { title: string; rows: BatteryExtraRow[] }[] = [];
	let title = "Extra";
	let buf: BatteryExtraRow[] = [];
	for (const r of rows) {
		if (r.label.startsWith("§ ")) {
			if (buf.length) {
				sections.push({ title, rows: buf });
				buf = [];
			}
			title = r.label.slice(2).trim() || "Extra";
		} else {
			buf.push(r);
		}
	}
	if (buf.length || sections.length === 0) {
		sections.push({ title, rows: buf });
	}
	return sections;
}

@customElement("app-advanced")
export class AppAdvanced extends LitElement {
	@state() private cmds: BatteryCommandDef[] = [];
	@state() private extra: BatteryExtraEntry[] = [];
	@state() private err = "";
	@state() private last = "";

	static styles = [
		sharedStyles,
		css`
			.cmd {
				display: flex;
				flex-wrap: wrap;
				align-items: center;
				gap: 10px;
				background: var(--color-surface);

				border-radius: var(--radius-control);
				padding: 10px 14px;
				margin-bottom: 8px;
			}
			.cmd strong {
				color: var(--high-emphasis);
				font-size: 0.9rem;
			}
			.cmd .id {
				font-size: 0.75rem;
				color: var(--medium-emphasis);
				font-family: var(--font-mono);
			}
			.cmd .spacer {
				flex: 1;
			}
			.section-title {
				margin: 14px 0 6px;
				font-size: 0.8rem;
				font-weight: 650;
				color: var(--color-accent);
				text-transform: uppercase;
				letter-spacing: 0.04em;
				border-bottom: 1px solid var(--color-outline);
				padding-bottom: 4px;
			}
			.section-title:first-of-type {
				margin-top: 0;
			}
			dl {
				margin: 0;
				display: grid;
				grid-template-columns: minmax(8rem, 1fr) 2fr;
				gap: 4px 12px;
				font-size: 0.83rem;
			}
			dt {
				color: var(--medium-emphasis);
				font-weight: 500;
			}
			dd {
				margin: 0;
				word-break: break-word;
				color: var(--high-emphasis);
			}
		`
	];

	connectedCallback(): void {
		super.connectedCallback();
		this.load();
	}

	async load() {
		const [d, e] = await api.battery.commands();
		const [ex, e2] = await api.battery.extra();
		const parts = [e?.error_msg || e, e2?.error_msg || e2].filter(Boolean);
		this.err = parts.map(String).join(" ");
		this.cmds = e || !d ? [] : d.commands;
		this.extra = !e2 && ex?.batteries ? ex.batteries : [];
	}

	async run(cmd: BatteryCommandDef, batt: number) {
		if (cmd.confirm && !confirm(cmd.confirm)) return;
		const [, err] = await api.battery.command(cmd.id, batt);
		this.last = err ? `Error: ${err.error_msg || err}` : `OK: ${cmd.id} on battery ${batt}`;
	}

	render() {
		return html`
			<div class="page-header">
				<h2>Advanced battery</h2>
				<div class="spacer"></div>
				<ct-button flat @click=${this.load}>
					<ct-icon slot="prefix" .svg=${refreshSvg}></ct-icon>
					Reload commands
				</ct-button>
			</div>
			${this.err ? html`<p class="err">${this.err}</p>` : null} ${this.last ? html`<p class="${this.last.startsWith("Error") ? "err" : "ok"}">${this.last}</p>` : null}
			${this.extra.map((b) => this.renderExtraBattery(b))}
			${this.cmds.map(
				(c) => html`
					<div class="cmd">
						<strong>${c.label}</strong>
						<span class="id">${c.id}</span>
						<span class="spacer"></span>
						${c.batteries.map((bn) => html` <ct-button light @click=${() => this.run(c, bn)}>Battery ${bn}</ct-button> `)}
					</div>
				`
			)}
		`;
	}

	private renderExtraBattery(b: BatteryExtraEntry) {
		const rows = b.rows ?? [];
		const sections = groupExtraRows(rows);
		return html`
			<ct-card>
				<h3>Battery ${b.index} · extra status</h3>
				${rows.length === 0
					? html`<p class="empty">(no rows)</p>`
					: sections.map(
							(s) => html`
								<div class="section">
									<div class="section-title">${s.title}</div>
									${s.rows.length === 0
										? html`<p class="empty">(empty)</p>`
										: html`<dl>
												${s.rows.map(
													(r) => html`
														<dt>${r.label}</dt>
														<dd>${r.value}</dd>
													`
												)}
											</dl>`}
								</div>
							`
						)}
			</ct-card>
		`;
	}
}
