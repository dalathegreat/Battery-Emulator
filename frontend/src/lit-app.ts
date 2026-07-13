import { CtIcon } from "@conectate/components/ct-icon";
import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import "./components/shell/app-schema.js";
import { BatteryEmulatorRouterController } from "./router-controller.js";
import { injectTheme } from "./styles/theme.js";

if (import.meta.env.DEV && import.meta.env.VITE_MOCK === "true") {
	await import("./mocks/index.js");
}

// Fully offline device UI: never fetch icon fonts from Google Fonts.
// All icons are inline SVGs passed to <ct-icon .svg=...>.
CtIcon.loadFonts = () => undefined;

@customElement("lit-app")
export class LitApp extends LitElement {
	@state() public islogged = true;
	@state() public currentView: any = null;
	private controller = new BatteryEmulatorRouterController(this);

	static styles = css`
		:host {
			display: block;
			min-height: 100dvh;
		}
	`;

	connectedCallback(): void {
		super.connectedCallback();
		injectTheme();
		this.controller.setupRouter();
	}

	render() {
		return html`<app-schema>${this.currentView}</app-schema>`;
	}
}

declare global {
	interface HTMLElementTagNameMap {
		"lit-app": LitApp;
	}
}
