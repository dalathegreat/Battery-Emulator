import "@conectate/components/ct-button";
import "@conectate/components/ct-icon";
import "@conectate/components/ct-spinner";
import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { api } from "../api/index.js";
import refreshSvg from "../icons/refresh.svg";
import restartSvg from "../icons/restart.svg";
import saveSvg from "../icons/save.svg";
import { sharedStyles } from "../styles/shared-styles.js";
import type { SettingsResponse, SettingsSelectOption } from "../types/settings.types.js";
import { xget, xhttp } from "../xFetch.js";

@customElement("app-settings")
export class AppSettings extends LitElement {
	@state() private data: SettingsResponse | null = null;
	@state() private err = "";
	@state() private busy = false;
	@state() private msg = "";

	static styles = [
		sharedStyles,
		css`
			fieldset {
				border-radius: var(--radius-card);
				margin: 0 0 16px;
				padding: 14px 16px 16px;
				background: var(--color-surface);
			}
			legend {
				padding: 0 8px;
				font-size: 0.85rem;
				font-weight: 650;
				color: var(--color-primary);
				text-transform: uppercase;
				letter-spacing: 0.04em;
			}
			label {
				display: flex;
				flex-wrap: wrap;
				align-items: center;
				gap: 8px;
				margin: 8px 0;
				font-size: 0.83rem;
				color: var(--medium-emphasis);
			}
			input[type="text"],
			input[type="number"],
			input[type="password"] {
				flex: 1;
				min-width: 120px;
				max-width: 320px;
			}
			select {
				flex: 1;
				min-width: 160px;
				max-width: min(100%, 420px);
			}
			.checks {
				display: grid;
				grid-template-columns: repeat(auto-fill, minmax(220px, 1fr));
				gap: 2px 16px;
			}
			.checks label {
				margin: 4px 0;
			}
			.footer-actions {
				position: sticky;
				bottom: 0;
				display: flex;
				flex-wrap: wrap;
				gap: 8px;
				padding: 12px 0;
				background: linear-gradient(to top, var(--color-background) 70%, transparent);
			}
		`
	];

	connectedCallback(): void {
		super.connectedCallback();
		const q = new URLSearchParams(window.location.search);
		if (q.get("settingsSaved") === "1") {
			this.msg = "Settings saved. Reboot may be required for some changes.";
			window.history.replaceState({}, "", window.location.pathname);
		}
		this.load();
	}

	async load() {
		const [d, e] = await api.settings.get();
		this.err = e ? String(e.error_msg || e) : "";
		this.data = e ? null : d;
	}

	private onSubmit(e: Event) {
		e.preventDefault();
		const form = e.target as HTMLFormElement;
		void this.submitForm(new FormData(form));
	}

	private async submitForm(fd: FormData) {
		this.busy = true;
		this.msg = "";
		const [, err] = await api.settings.save(fd);
		this.busy = false;
		if (err) {
			this.msg = String(err.error_msg || err);
			return;
		}
		this.msg = "Saved. Redirecting…";
		window.location.assign("/settings?settingsSaved=1");
	}

	private async factoryReset() {
		if (!confirm("Factory reset all NVM settings?")) return;
		this.busy = true;
		const [, err] = await xhttp<string>("/factoryReset", "POST");
		this.busy = false;
		this.msg = err ? String(err.error_msg || err) : "Factory reset OK — reboot device.";
	}

	/** Toggle between the new SPA and the legacy HTML UI (applies after reboot). */
	private async toggleNewWebUI() {
		const current = this.data?.bool?.NEWWEBUI ?? true;
		const next = current ? 0 : 1;
		const label = next ? "new web UI" : "legacy web UI";
		if (!confirm(`Switch to the ${label}? A reboot is required to apply the change.`)) return;
		this.busy = true;
		const [, err] = await xget<string>("/enableNewWebUI", { value: next });
		this.busy = false;
		if (err) {
			this.msg = String(err.error_msg || err);
			return;
		}
		this.msg = `Switched to ${label}. Reboot to apply.`;
		void this.load();
	}

	private async reboot() {
		if (!confirm("Reboot the emulator? If it is handling contactors, they will open during reboot.")) return;
		this.busy = true;
		const [, err] = await xget<string>("/reboot", undefined, { retries: 1 });
		this.busy = false;
		this.msg = err ? String(err.error_msg || err) : "Rebooting… the device will be back in a few seconds.";
	}

	/** Renders a &lt;select&gt; when the API provides `select_options`; otherwise a number field. */
	private renderSelect(name: string, caption: string, current: number, options: SettingsSelectOption[] | undefined) {
		if (!options?.length) {
			return html`<label>${caption} <input type="number" name=${name} .value=${String(current)} /></label>`;
		}
		const rows = [...options];
		if (!rows.some((r) => Number(r.value) === Number(current))) {
			rows.unshift({ value: current, label: `(saved value ${current})` });
		}
		return html`<label
			>${caption}
			<select name=${name}>
				${rows.map((o) => html`<option value=${String(o.value)} ?selected=${Number(o.value) === Number(current)}>${o.label}</option>`)}
			</select></label
		>`;
	}

	render() {
		if (!this.data && !this.err) return html`<div class="state"><ct-spinner></ct-spinner><span>Loading settings…</span></div>`;
		if (this.err)
			return html`<div class="state">
				<p class="err">${this.err}</p>
				<ct-button raised @click=${this.load}>Retry</ct-button>
			</div>`;
		const d = this.data!;
		// NEWWEBUI has its own toggle (via /enableNewWebUI); keep it out of the generic form
		const boolKeys = Object.keys(d.bool)
			.filter((k) => k !== "NEWWEBUI")
			.sort();
		const newWebUI = d.bool.NEWWEBUI ?? true;
		const uintKeys = Object.keys(d.uint)
			.filter((k) => k !== "BATTPVMAX" && k !== "BATTPVMIN")
			.sort();
		const strKeys = Object.keys(d.string)
			.filter((k) => k !== "SSID")
			.sort();
		const sel = d.select;
		const o = d.select_options;
		const batpMaxV = (d.uint.BATTPVMAX ?? 0) / 10;
		const batpMinV = (d.uint.BATTPVMIN ?? 0) / 10;

		return html`
			<div class="page-header">
				<h2>Settings</h2>
				<p class="subtitle">Submits <code>multipart/form-data</code> to <code>POST /saveSettings</code>. Choices come from the device when <code>select_options</code> is present.</p>
			</div>
			${this.msg ? html`<p class="ok">${this.msg}</p>` : null}
			<form @submit=${this.onSubmit}>
				<fieldset>
					<legend>WiFi / MQTT</legend>
					<label>SSID <input type="text" name="SSID" .value=${d.string.SSID ?? ""} /></label>
					<label>Password <input type="password" name="PASSWORD" autocomplete="new-password" placeholder="(unchanged if empty)" /></label>
					<label>MQTT publish interval (s) <input type="number" name="MQTTPUBLISHMS" min="1" step="1" .value=${String(d.mqtt_publish_s)} /></label>
				</fieldset>

				<fieldset>
					<legend>Protocols</legend>
					${this.renderSelect("inverter", "Inverter protocol", sel.INVTYPE ?? 0, o?.inverter)} ${this.renderSelect("INVCOMM", "Inverter communication", sel.INVCOMM ?? 0, o?.comm)}
					${this.renderSelect("battery", "Battery type", sel.BATTTYPE ?? 0, o?.battery)} ${this.renderSelect("BATTCHEM", "Battery chemistry", sel.BATTCHEM ?? 0, o?.chemistry)}
					${this.renderSelect("BATTCOMM", "Battery communication", sel.BATTCOMM ?? 0, o?.comm)} ${this.renderSelect("charger", "Charger type", sel.CHGTYPE ?? 0, o?.charger)}
					${this.renderSelect("CHGCOMM", "Charger communication", sel.CHGCOMM ?? 0, o?.comm)} ${this.renderSelect("EQSTOP", "Equipment stop button", sel.EQSTOP ?? 0, o?.eqstop)}
					${this.renderSelect("BATT2COMM", "Battery 2 communication", sel.BATT2COMM ?? 0, o?.comm)}
					${this.renderSelect("BATT3COMM", "Battery 3 communication", sel.BATT3COMM ?? 0, o?.comm)}
					${this.renderSelect("shunttype", "Shunt / current sensor", sel.SHUNTTYPE ?? 0, o?.shunt)}
					${this.renderSelect("SHUNTCOMM", "Shunt communication", sel.SHUNTCOMM ?? 0, o?.comm)}
					${this.renderSelect("CTATTEN", "CT clamp ADC attenuation", sel.CTATTEN ?? 0, o?.ctatten)}
				</fieldset>

				<fieldset>
					<legend>Pack voltage limits (volts)</legend>
					<label>Max pack V <input type="number" name="BATTPVMAX" step="0.1" .value=${String(batpMaxV)} /></label>
					<label>Min pack V <input type="number" name="BATTPVMIN" step="0.1" .value=${String(batpMinV)} /></label>
					<label>CT offset (string) <input type="text" name="CTOFFSET" .value=${d.string_extra?.CTOFFSET ?? ""} /></label>
				</fieldset>

				<fieldset>
					<legend>Unsigned integers</legend>
					${uintKeys.map((k) => html`<label>${k} <input type="number" name=${k} .value=${String(d.uint[k] ?? 0)} /></label>`)}
				</fieldset>

				<fieldset>
					<legend>Strings</legend>
					${strKeys.map((k) => html`<label>${k} <input type="text" name=${k} .value=${d.string[k] ?? ""} /></label>`)}
				</fieldset>

				<fieldset>
					<legend>Booleans</legend>
					<div class="checks">${boolKeys.map((k) => html`<label><input type="checkbox" name=${k} value="on" ?checked=${d.bool[k]} />${k}</label>`)}</div>
				</fieldset>

				<div class="footer-actions">
					<ct-button gap raised ?disabled=${this.busy} @click=${() => this.shadowRoot?.querySelector("form")?.requestSubmit()}>
						<ct-icon slot="prefix" .svg=${saveSvg}></ct-icon>
						Save settings
					</ct-button>
					<ct-button gap flat ?disabled=${this.busy} @click=${this.load}>
						<ct-icon slot="prefix" .svg=${refreshSvg}></ct-icon>
						Reload
					</ct-button>
					<ct-button gap light ?disabled=${this.busy} @click=${this.toggleNewWebUI}> ${newWebUI ? "Switch to legacy web UI" : "Use new web UI"} </ct-button>
					<ct-button gap light ?disabled=${this.busy} @click=${this.reboot}>
						<ct-icon slot="prefix" .svg=${restartSvg}></ct-icon>
						Reboot
					</ct-button>
					<ct-button gap light style="--color-primary: var(--color-error)" ?disabled=${this.busy} @click=${this.factoryReset}>
						<ct-icon slot="prefix" .svg=${restartSvg}></ct-icon>
						Factory reset
					</ct-button>
				</div>
			</form>
		`;
	}
}
