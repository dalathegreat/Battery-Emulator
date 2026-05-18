import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { BatteryEmulatorRouterController } from "../router-controller.js";
import "./app-advanced.js";
import "./app-canlog.js";
import "./app-cellmonitor.js";
import "./app-dashboard.js";
import "./app-debuglog.js";
import "./app-events.js";
import "./app-settings.js";

type Route = "dashboard" | "cellmonitor" | "settings" | "events" | "canlog" | "debuglog" | "advanced";

function routeFromHash(): Route {
	const h = (location.hash || "#dashboard").slice(1).toLowerCase();
	const allowed: Route[] = ["dashboard", "cellmonitor", "settings", "events", "canlog", "debuglog", "advanced"];
	return (allowed.includes(h as Route) ? h : "dashboard") as Route;
}

@customElement("app-root")
export class AppRoot extends LitElement {
	@state() private route: Route = routeFromHash();
	@state() public islogged = true;
	@state() public currentView: any = null;
	private controller = new BatteryEmulatorRouterController(this);

	static styles = css`
		:host {
			display: block;
			min-height: 100vh;
			background: #000;
			color: #fff;
		}
		nav {
			display: flex;
			flex-wrap: wrap;
			gap: 0.35rem;
			padding: 0.75rem;
			background: #1e2c33;
			position: sticky;
			top: 0;
			z-index: 10;
		}
		nav a {
			color: #fff;
			text-decoration: none;
			padding: 0.4rem 0.75rem;
			border-radius: 8px;
			background: #394b52;
			font-size: 14px;
		}
		nav a.active {
			background: #505e67;
		}
		main {
			padding: 1rem;
			max-width: 1200px;
			margin: 0 auto;
		}
	`;

	connectedCallback(): void {
		super.connectedCallback();
		this.controller.setupRouter();
	}

	private link(r: Route, label: string) {
		const active = this.route === r ? "active" : "";
		return html`<a href="/${r}" class=${active}>${label}</a>`;
	}

	render() {
		return html`
			<nav>
				${this.link("dashboard", "Dashboard")} ${this.link("cellmonitor", "Cells")} ${this.link("settings", "Settings")} ${this.link("events", "Events")}
				${this.link("canlog", "CAN log")} ${this.link("debuglog", "Debug log")} ${this.link("advanced", "Advanced")}
			</nav>
			<main>${this.currentView}</main>
		`;
	}
}
