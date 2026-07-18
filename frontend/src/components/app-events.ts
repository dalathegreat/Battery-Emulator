import "@conectate/components/ct-button";
import "@conectate/components/ct-icon";
import "@conectate/components/ct-spinner";
import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { api } from "../api/index.js";
import deleteSvg from "../icons/delete.svg";
import refreshSvg from "../icons/refresh.svg";
import { sharedStyles } from "../styles/shared-styles.js";
import type { EventRow } from "../types/events.types.js";

@customElement("app-events")
export class AppEvents extends LitElement {
	@state() private rows: EventRow[] = [];
	@state() private err = "";
	@state() private loaded = false;

	static styles = [
		sharedStyles,
		css`
			.table-card {
				background: var(--color-surface);

				border-radius: var(--radius-card);
				overflow: auto;
			}
			table {
				width: 100%;
				border-collapse: collapse;
				font-size: 0.83rem;
			}
			th,
			td {
				padding: 10px 12px;
				text-align: left;
				border-bottom: 1px solid var(--color-outline);
			}
			th {
				background: var(--color-surface-bright);
				color: var(--medium-emphasis);
				font-size: 0.72rem;
				text-transform: uppercase;
				letter-spacing: 0.05em;
				position: sticky;
				top: 0;
			}
			tr:last-child td {
				border-bottom: none;
			}
			td.num {
				font-family: var(--font-mono);
				white-space: nowrap;
			}
		`
	];

	connectedCallback(): void {
		super.connectedCallback();
		this.load();
	}

	async load() {
		const [d, e] = await api.events.list();
		this.err = e ? String(e.error_msg || e) : "";
		this.rows = e || !d ? [] : d.events;
		this.loaded = true;
	}

	async clear() {
		if (!confirm("Clear all events?")) return;
		const [, err] = await api.events.clear();
		if (err) alert(String(err.error_msg || err));
		await this.load();
	}

	private sevChip(severity: string) {
		const cls = severity === "ERROR" ? "error" : severity === "WARNING" ? "warn" : "ok";
		return html`<span class="chip ${cls}">${severity}</span>`;
	}

	render() {
		if (!this.loaded) return html`<div class="state"><ct-spinner></ct-spinner><span>Loading events…</span></div>`;
		return html`
			<div class="page-header">
				<h2>Events</h2>
				<div class="spacer"></div>
				<ct-button flat @click=${this.load}>
					<ct-icon slot="prefix" .svg=${refreshSvg}></ct-icon>
					Refresh
				</ct-button>
				<ct-button light style="--color-primary: var(--color-error)" @click=${this.clear}>
					<ct-icon slot="prefix" .svg=${deleteSvg}></ct-icon>
					Clear all
				</ct-button>
			</div>
			${this.err ? html`<p class="err">${this.err}</p>` : null}
			${this.rows.length === 0
				? html`<div class="state"><span class="empty">No events recorded.</span></div>`
				: html`
						<div class="table-card">
							<table>
								<thead>
									<tr>
										<th>Type</th>
										<th>Severity</th>
										<th>Age (ms)</th>
										<th>Count</th>
										<th>Data</th>
										<th>Message</th>
									</tr>
								</thead>
								<tbody>
									${this.rows.map(
										(r) => html`
											<tr>
												<td>${r.type}</td>
												<td>${this.sevChip(r.severity)}</td>
												<td class="num">${r.age_ms}</td>
												<td class="num">${r.count}</td>
												<td class="num">${r.data}</td>
												<td>${r.message}</td>
											</tr>
										`
									)}
								</tbody>
							</table>
						</div>
					`}
		`;
	}
}
