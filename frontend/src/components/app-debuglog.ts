import "@conectate/components/ct-button";
import "@conectate/components/ct-icon";
import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { api } from "../api/index.js";
import downloadSvg from "../icons/download.svg";
import refreshSvg from "../icons/refresh.svg";
import { sharedStyles } from "../styles/shared-styles.js";

@customElement("app-debuglog")
export class AppDebuglog extends LitElement {
	@state() private text = "";
	@state() private err = "";
	@state() private poll: ReturnType<typeof setInterval> | null = null;

	static styles = [
		sharedStyles,
		css`
			pre.console {
				max-height: 70vh;
			}
		`
	];

	connectedCallback(): void {
		super.connectedCallback();
		void this.pull();
		this.poll = setInterval(() => void this.pull(), 2000);
	}

	disconnectedCallback(): void {
		super.disconnectedCallback();
		if (this.poll) clearInterval(this.poll);
	}

	async pull() {
		const [d, e] = await api.debuglog.get();
		if (e) {
			this.err = String(e.error_msg || e);
			return;
		}
		this.err = "";
		this.text = typeof d?.text === "string" ? d.text : "";
	}

	render() {
		return html`
			<div class="page-header">
				<h2>Debug log</h2>
				<div class="spacer"></div>
				<ct-button flat @click=${this.pull}>
					<ct-icon slot="prefix" .svg=${refreshSvg}></ct-icon>
					Refresh now
				</ct-button>
				<a href="/export_log" target="_blank" rel="noopener" style="text-decoration:none">
					<ct-button light>
						<ct-icon slot="prefix" .svg=${downloadSvg}></ct-icon>
						Export
					</ct-button>
				</a>
				<p class="subtitle">Polling <code>GET /api/debug/log</code> every 2 seconds.</p>
			</div>
			${this.err ? html`<p class="err">${this.err}</p>` : null}
			<pre class="console">${this.text || "—"}</pre>
		`;
	}
}
