import { LitElement, css, html } from "lit";
import { customElement, property, query, state } from "lit/decorators.js";
import { api } from "../../api/index.js";
import "./app-drawer.js";
import "./app-toolbar.js";
import type { ToolbarStatus } from "./app-toolbar.js";

const DESKTOP_MQ = "(min-width: 777px)";

/**
 * Application shell: fixed drawer on desktop, overlay drawer on mobile,
 * sticky toolbar with live emulator status, scrollable content section.
 */
@customElement("app-schema")
export class AppSchema extends LitElement {
	@property({ type: Boolean, reflect: true }) open = matchMedia(DESKTOP_MQ).matches;
	@state() private route = location.pathname;
	@state() private status: ToolbarStatus | null = null;
	@query("app-drawer") private drawer?: HTMLElementTagNameMap["app-drawer"];

	private statusTimer: ReturnType<typeof setInterval> | null = null;
	private onLocationChanged = () => {
		this.route = location.pathname;
		if (!matchMedia(DESKTOP_MQ).matches) this.open = false;
	};

	static styles = css`
		:host {
			display: flex;
			flex-direction: column;
			min-height: 100dvh;
			--drawer-width: 248px;
		}
		main {
			display: flex;
			flex-direction: column;
			min-height: 100dvh;
			transition: margin-left 200ms ease;
		}
		section {
			flex: 1;
			box-sizing: border-box;
			padding: 20px 16px 40px;
			width: 100%;
			max-width: 1200px;
			margin: 0 auto;
		}
		@media (min-width: 777px) {
			:host([open]) main {
				margin-left: var(--drawer-width);
			}
			section {
				padding: 28px 28px 48px;
			}
		}
	`;

	connectedCallback(): void {
		super.connectedCallback();
		window.addEventListener("location-changed", this.onLocationChanged);
		void this.pullStatus();
		this.statusTimer = setInterval(() => void this.pullStatus(), 15000);
	}

	disconnectedCallback(): void {
		window.removeEventListener("location-changed", this.onLocationChanged);
		if (this.statusTimer) clearInterval(this.statusTimer);
		super.disconnectedCallback();
	}

	private async pullStatus() {
		const [d, e] = await api.dashboard.get();
		if (e || !d) return;
		this.status = {
			emulator_running: !d.contactor.emulator_pause_request_on && !d.contactor.equipment_stop_active,
			emulator_text: d.ui.emulator_status,
			wifi_connected: d.wifi.connected,
			wifi_rssi: d.wifi.rssi
		};
		this.drawer?.setVersion(`${d.version} · ${d.hardware}`);
	}

	render() {
		return html`
			<app-drawer .open=${this.open} .route=${this.route} @open=${(e: CustomEvent<{ open: boolean }>) => (this.open = e.detail.open)}></app-drawer>
			<main>
				<app-toolbar .route=${this.route} .status=${this.status} @toggle=${() => (this.open = !this.open)}></app-toolbar>
				<section><slot></slot></section>
			</main>
		`;
	}
}

declare global {
	interface HTMLElementTagNameMap {
		"app-schema": AppSchema;
	}
}
