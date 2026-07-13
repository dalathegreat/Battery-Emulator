import "@conectate/components/ct-button";
import "@conectate/components/ct-card";
import "@conectate/components/ct-icon";
import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { api } from "../api/index.js";
import playSvg from "../icons/play.svg";
import refreshSvg from "../icons/refresh.svg";
import repeatSvg from "../icons/repeat.svg";
import stopSvg from "../icons/stop.svg";
import stopCircleSvg from "../icons/stop_circle.svg";
import visibilitySvg from "../icons/visibility.svg";
import { sharedStyles } from "../styles/shared-styles.js";
import type { CanlogStatusResponse } from "../types/system.types.js";
import { getAPIBaseURL } from "../xFetch.js";

@customElement("app-canlog")
export class AppCanlog extends LitElement {
	@state() private st: CanlogStatusResponse | null = null;
	@state() private msg = "";
	@state() private err = "";
	@state() private logText = "";
	@state() private cutoffInput = "";
	@state() private viewerOn = false;
	@state() private poll: ReturnType<typeof setInterval> | null = null;

	static styles = [
		sharedStyles,
		css`
			label {
				display: inline-flex;
				align-items: center;
				gap: 8px;
				margin: 6px 8px 6px 0;
				font-size: 0.83rem;
				color: var(--medium-emphasis);
			}
			input[type="number"] {
				width: 6.5rem;
			}
			.actions {
				display: flex;
				flex-wrap: wrap;
				align-items: center;
				gap: 8px;
			}
			input[type="file"] {
				font-size: 0.83rem;
				color: var(--medium-emphasis);
			}
		`
	];

	connectedCallback(): void {
		super.connectedCallback();
		void this.load();
	}

	disconnectedCallback(): void {
		super.disconnectedCallback();
		if (this.poll) clearInterval(this.poll);
	}

	async load() {
		const [d, e] = await api.canlog.status();
		this.err = e ? String(e.error_msg || e) : "";
		this.st = e ? null : d;
	}

	async enableViewer() {
		const [, e] = await api.canlog.enableViewer();
		if (e) {
			this.msg = String(e.error_msg || e);
			return;
		}
		this.viewerOn = true;
		this.msg = "Web CAN viewer enabled.";
		if (this.poll) clearInterval(this.poll);
		void this.pullMessages();
		this.poll = setInterval(() => void this.pullMessages(), 1500);
	}

	async pullMessages() {
		const [d, e] = await api.canlog.messages();
		if (e) return;
		this.logText = d?.text ?? "";
		if (d && this.cutoffInput === "") this.cutoffInput = String(d.can_id_cutoff ?? 0);
	}

	async applyCutoff() {
		const v = parseInt(this.cutoffInput, 10);
		if (Number.isNaN(v)) return;
		const [, e] = await api.canlog.setCanIdCutoff(v);
		this.msg = e ? String(e.error_msg || e) : `Cutoff set to ${v}`;
	}

	async replayStart(loop: boolean) {
		const [t, e] = await api.canlog.startReplay(loop);
		this.msg = e ? String(e.error_msg || e) : String(t);
	}

	async replayStop() {
		const [t, e] = await api.canlog.stopReplay();
		this.msg = e ? String(e.error_msg || e) : String(t);
		await this.load();
	}

	private onFile(e: Event) {
		const inp = e.target as HTMLInputElement;
		const f = inp.files?.[0];
		if (!f) return;
		const fd = new FormData();
		fd.append("file", f, f.name);
		void (async () => {
			const res = await fetch(`${getAPIBaseURL()}/import_can_log`, {
				method: "POST",
				body: fd,
				credentials: "include"
			});
			this.msg = res.ok ? "Upload started / OK" : `Upload failed ${res.status}`;
			inp.value = "";
		})();
	}

	render() {
		return html`
			<div class="page-header">
				<h2>CAN log / replay</h2>
				<div class="spacer"></div>
				<ct-button flat @click=${this.load}>
					<ct-icon slot="prefix" .svg=${refreshSvg}></ct-icon>
					Refresh status
				</ct-button>
			</div>
			${this.err ? html`<p class="err">${this.err}</p>` : null} ${this.msg ? html`<p class="ok">${this.msg}</p>` : null}
			${this.st
				? html`
						<ct-card>
							<h3>Status</h3>
							<pre class="console">${JSON.stringify(this.st, null, 2)}</pre>
						</ct-card>
					`
				: null}
			<ct-card>
				<h3>Replay</h3>
				<label
					>Replay interface (0–4)
					<input
						type="number"
						min="0"
						max="4"
						.value=${String(this.st?.can_replay_interface ?? 0)}
						@change=${(e: Event) => {
							const v = (e.target as HTMLInputElement).value;
							void api.canlog.setCANInterface(Number(v));
						}}
				/></label>
				<div class="actions">
					<ct-button raised @click=${() => this.replayStart(false)}>
						<ct-icon slot="prefix" .svg=${playSvg}></ct-icon>
						Start replay
					</ct-button>
					<ct-button flat @click=${() => this.replayStart(true)}>
						<ct-icon slot="prefix" .svg=${repeatSvg}></ct-icon>
						Start replay (loop)
					</ct-button>
					<ct-button light style="--color-primary: var(--color-warning)" @click=${this.replayStop}>
						<ct-icon slot="prefix" .svg=${stopSvg}></ct-icon>
						Stop replay
					</ct-button>
				</div>
			</ct-card>
			<ct-card>
				<h3>Web viewer</h3>
				<div class="actions">
					<ct-button raised @click=${this.enableViewer}>
						<ct-icon slot="prefix" .svg=${visibilitySvg}></ct-icon>
						Enable logging + viewer
					</ct-button>
					<ct-button light style="--color-primary: var(--color-warning)" @click=${() => api.canlog.stopCanLogging()}>
						<ct-icon slot="prefix" .svg=${stopCircleSvg}></ct-icon>
						Stop CAN logging
					</ct-button>
					<label
						>CAN ID cutoff
						<input
							type="number"
							.value=${this.cutoffInput}
							@input=${(e: Event) => {
								this.cutoffInput = (e.target as HTMLInputElement).value;
							}}
					/></label>
					<ct-button flat @click=${this.applyCutoff}>Apply cutoff</ct-button>
				</div>
				<p>
					<a href="/export_can_log" target="_blank" rel="noopener">Export log</a> ·
					<a href="/delete_can_log" target="_blank" rel="noopener">Delete log</a>
				</p>
				<p class="hint">Import log (multipart to <code>/import_can_log</code>):</p>
				<input type="file" @change=${this.onFile} />
				${this.viewerOn ? html`<pre class="console" style="margin-top:12px">${this.logText || "—"}</pre>` : null}
			</ct-card>
		`;
	}
}
