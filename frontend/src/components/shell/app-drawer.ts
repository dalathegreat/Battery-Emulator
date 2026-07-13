import "@conectate/components/ct-icon";
import { LitElement, css, html } from "lit";
import { customElement, property, state } from "lit/decorators.js";
import batterySvg from "../../icons/battery.svg";
import boltSvg from "../../icons/bolt.svg";
import cellsSvg from "../../icons/cells.svg";
import dashboardSvg from "../../icons/dashboard.svg";
import debugSvg from "../../icons/debug.svg";
import settingsSvg from "../../icons/settings.svg";
import terminalSvg from "../../icons/terminal.svg";
import warningSvg from "../../icons/warning.svg";

interface NavItem {
	path: string;
	label: string;
	svg: string;
}

export const NAV_ITEMS: NavItem[] = [
	{ path: "/", label: "Dashboard", svg: dashboardSvg },
	{ path: "/cellmonitor", label: "Cell monitor", svg: cellsSvg },
	{ path: "/settings", label: "Settings", svg: settingsSvg },
	{ path: "/events", label: "Events", svg: warningSvg },
	{ path: "/canlog", label: "CAN log", svg: terminalSvg },
	{ path: "/debuglog", label: "Debug log", svg: debugSvg },
	{ path: "/advanced", label: "Advanced", svg: boltSvg }
];

@customElement("app-drawer")
export class AppDrawer extends LitElement {
	@property({ type: Boolean, reflect: true }) open = false;
	@property({ type: String }) route = "/";
	@state() private version = "";

	static styles = css`
		:host {
			display: block;
			--ct-icon-size: 20px;
		}
		aside {
			position: fixed;
			top: 0;
			left: 0;
			bottom: 0;
			width: var(--drawer-width, 248px);
			background: var(--color-surface);
			border-right: 1px solid var(--color-outline);
			box-sizing: border-box;
			display: flex;
			flex-direction: column;
			z-index: var(--zi-drawer, 20);
			transform: translateX(-100%);
			transition: transform 200ms ease;
			overflow-y: auto;
		}
		:host([open]) aside {
			transform: translateX(0);
		}
		.scrim {
			position: fixed;
			inset: 0;
			background: var(--color-scrim);
			z-index: var(--zi-scrim, 18);
			opacity: 0;
			pointer-events: none;
			transition: opacity 200ms ease;
		}
		:host([open]) .scrim {
			opacity: 1;
			pointer-events: auto;
		}
		@media (min-width: 777px) {
			.scrim {
				display: none;
			}
		}

		.brand {
			display: flex;
			align-items: center;
			gap: 12px;
			padding: 18px 16px 14px;
		}
		.brand .logo {
			display: grid;
			place-items: center;
			width: 38px;
			height: 38px;
			border-radius: 11px;
			background: var(--color-primary-light);
			color: var(--color-primary);
			--ct-icon-size: 22px;
		}
		.brand .name {
			font-weight: 700;
			font-size: 0.95rem;
			color: var(--high-emphasis);
			line-height: 1.2;
		}
		.brand .sub {
			font-size: 0.7rem;
			color: var(--medium-emphasis);
		}

		nav {
			flex: 1;
			padding: 6px 10px;
			display: flex;
			flex-direction: column;
			gap: 2px;
		}
		nav a {
			display: flex;
			align-items: center;
			gap: 12px;
			padding: 9px 12px;
			border-radius: 10px;
			color: var(--medium-emphasis);
			text-decoration: none;
			font-size: 0.87rem;
			font-weight: 500;
			transition:
				background 120ms ease,
				color 120ms ease;
		}
		nav a ct-icon {
			flex-shrink: 0;
		}
		nav a:hover {
			background: var(--color-surface-bright);
			color: var(--high-emphasis);
		}
		nav a.active {
			background: var(--color-primary-light);
			color: var(--color-primary);
			font-weight: 650;
		}

		footer {
			padding: 12px 16px;
			border-top: 1px solid var(--color-outline);
			font-size: 0.7rem;
			color: var(--medium-emphasis);
			overflow-wrap: anywhere;
		}
	`;

	setVersion(version: string) {
		this.version = version;
	}

	private isActive(path: string) {
		return path === "/" ? this.route === "/" : this.route.startsWith(path);
	}

	private close() {
		this.dispatchEvent(new CustomEvent("open", { detail: { open: false }, bubbles: true, composed: true }));
	}

	render() {
		return html`
			<div class="scrim" @click=${this.close}></div>
			<aside>
				<div class="brand">
					<div class="logo"><ct-icon .svg=${batterySvg}></ct-icon></div>
					<div>
						<div class="name">Battery Emulator</div>
						<div class="sub">Control panel</div>
					</div>
				</div>
				<nav>
					${NAV_ITEMS.map(
						(item) => html`
							<a href=${item.path} class=${this.isActive(item.path) ? "active" : ""}>
								<ct-icon .svg=${item.svg}></ct-icon>
								<span>${item.label}</span>
							</a>
						`
					)}
				</nav>
				<footer>${this.version || html`&nbsp;`}</footer>
			</aside>
		`;
	}
}

declare global {
	interface HTMLElementTagNameMap {
		"app-drawer": AppDrawer;
	}
}
