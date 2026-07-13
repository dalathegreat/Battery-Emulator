import "@conectate/components/ct-icon-button";
import { LitElement, css, html } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import menuSvg from "../../icons/menu.svg";
import moonSvg from "../../icons/moon.svg";
import sunSvg from "../../icons/sun.svg";
import { getTheme, toggleTheme } from "../../styles/theme.js";
import { NAV_ITEMS } from "./app-drawer.js";

export interface ToolbarStatus {
	emulator_running: boolean;
	emulator_text: string;
	wifi_connected: boolean;
	wifi_rssi?: number;
}

@customElement("app-toolbar")
export class AppToolbar extends LitElement {
	@property({ type: String }) route = "/";
	@property({ attribute: false }) status: ToolbarStatus | null = null;
	@state() private theme = getTheme();

	static styles = css`
		:host {
			display: block;
			position: sticky;
			top: 0;
			z-index: var(--zi-toolbar, 15);
			--ct-icon-size: 22px;
		}
		header {
			display: flex;
			align-items: center;
			gap: 8px;
			height: var(--toolbar-height, 52px);
			padding: 0 12px;
			background: color-mix(in srgb, var(--color-surface) 82%, transparent);
			backdrop-filter: blur(10px);
			-webkit-backdrop-filter: blur(10px);
			border-bottom: 1px solid var(--color-outline);
		}
		ct-icon-button {
			color: var(--color-on-surface);
		}
		.title {
			font-weight: 650;
			font-size: 0.98rem;
			color: var(--high-emphasis);
			white-space: nowrap;
			overflow: hidden;
			text-overflow: ellipsis;
		}
		.spacer {
			flex: 1;
		}
		.chip {
			display: inline-flex;
			align-items: center;
			gap: 6px;
			padding: 4px 11px;
			border-radius: 999px;
			font-size: 0.74rem;
			font-weight: 600;
			background: var(--color-surface-bright);

			color: var(--medium-emphasis);
			white-space: nowrap;
		}
		.chip .dot {
			width: 7px;
			height: 7px;
			border-radius: 50%;
			background: var(--color-disable);
		}
		.chip.ok {
			color: var(--color-success);
			background: var(--color-success-container);
			border-color: transparent;
		}
		.chip.ok .dot {
			background: var(--color-success);
			box-shadow: 0 0 6px var(--color-success);
		}
		.chip.warn {
			color: var(--color-warning);
			background: var(--color-warning-container);
			border-color: transparent;
		}
		.chip.warn .dot {
			background: var(--color-warning);
		}
		@media (max-width: 520px) {
			.chip.wifi {
				display: none;
			}
		}
	`;

	private get pageTitle() {
		const item = NAV_ITEMS.find((n) => (n.path === "/" ? this.route === "/" : this.route.startsWith(n.path)));
		return item?.label ?? "Battery Emulator";
	}

	private onToggleTheme() {
		this.theme = toggleTheme();
	}

	render() {
		const st = this.status;
		return html`
			<header>
				<ct-icon-button .svg=${menuSvg} aria-label="Menu" @click=${() => this.dispatchEvent(new CustomEvent("toggle", { bubbles: true, composed: true }))}></ct-icon-button>
				<div class="title">${this.pageTitle}</div>
				<div class="spacer"></div>
				${st
					? html`
							<span class="chip ${st.emulator_running ? "ok" : "warn"}">
								<span class="dot"></span>
								${st.emulator_text}
							</span>
							${st.wifi_connected && st.wifi_rssi != null ? html`<span class="chip wifi">WiFi ${st.wifi_rssi} dBm</span>` : null}
						`
					: null}
				<ct-icon-button .svg=${this.theme === "dark" ? sunSvg : moonSvg} aria-label="Toggle theme" @click=${this.onToggleTheme}></ct-icon-button>
			</header>
		`;
	}
}

declare global {
	interface HTMLElementTagNameMap {
		"app-toolbar": AppToolbar;
	}
}
