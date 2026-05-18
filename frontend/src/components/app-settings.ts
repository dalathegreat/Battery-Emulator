import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { api } from "../api/index.js";
import type { SettingsResponse, SettingsSelectOption } from "../types/settings.types.js";
import { xhttp } from "../xFetch.js";

@customElement("app-settings")
export class AppSettings extends LitElement {
  @state() private data: SettingsResponse | null = null;
  @state() private err = "";
  @state() private busy = false;
  @state() private msg = "";

  static styles = css`
    :host {
      display: block;
      color: #fff;
      font-family: system-ui, sans-serif;
    }
    fieldset {
      border: 1px solid #455a64;
      border-radius: 10px;
      margin: 0 0 1rem;
      padding: 0.75rem 1rem;
      background: #303e47;
    }
    legend {
      padding: 0 0.35rem;
    }
    label {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      gap: 0.35rem;
      margin: 0.35rem 0;
      font-size: 13px;
    }
    input[type="text"],
    input[type="number"],
    input[type="password"] {
      flex: 1;
      min-width: 120px;
      max-width: 320px;
      padding: 0.35rem 0.5rem;
      border-radius: 6px;
      border: 1px solid #555;
      background: #1a252b;
      color: #fff;
    }
    select {
      flex: 1;
      min-width: 160px;
      max-width: min(100%, 420px);
      padding: 0.35rem 0.5rem;
      border-radius: 6px;
      border: 1px solid #555;
      background: #1a252b;
      color: #fff;
    }
    input[type="checkbox"] {
      width: 1rem;
      height: 1rem;
    }
    button {
      background: #505e67;
      color: #fff;
      border: none;
      padding: 0.5rem 1rem;
      border-radius: 8px;
      cursor: pointer;
      margin: 0.25rem 0.5rem 0.25rem 0;
    }
    button.danger {
      background: #7a3030;
    }
    .err {
      color: #f88;
    }
    .ok {
      color: #8c8;
    }
    .hint {
      font-size: 12px;
      opacity: 0.75;
      margin-bottom: 0.5rem;
    }
  `;

  connectedCallback(): void {
    super.connectedCallback();
    const q = new URLSearchParams(window.location.search);
    if (q.get("settingsSaved") === "1") {
      this.msg = "Settings saved. Reboot may be required for some changes.";
      window.history.replaceState({}, "", window.location.pathname + window.location.hash);
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
    window.location.assign("/?settingsSaved=1#/settings");
  }

  private async factoryReset() {
    if (!confirm("Factory reset all NVM settings?")) return;
    this.busy = true;
    const [, err] = await xhttp<string>("/factoryReset", "POST");
    this.busy = false;
    this.msg = err ? String(err.error_msg || err) : "Factory reset OK — reboot device.";
  }

  /** Renders a &lt;select&gt; when the API provides `select_options`; otherwise a number field. */
  private renderSelect(
    name: string,
    caption: string,
    current: number,
    options: SettingsSelectOption[] | undefined
  ) {
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
        ${rows.map(
          (o) =>
            html`<option value=${String(o.value)} ?selected=${Number(o.value) === Number(current)}>
              ${o.label}
            </option>`
        )}
      </select></label
    >`;
  }

  render() {
    if (!this.data && !this.err) return html`<p>Loading…</p>`;
    if (this.err) return html`<p class="err">${this.err}</p><button type="button" @click=${this.load}>Retry</button>`;
    const d = this.data!;
    const boolKeys = Object.keys(d.bool).sort();
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
      <h2>Settings</h2>
      <p class="hint">
        Submits <code>multipart/form-data</code> to <code>POST /saveSettings</code>. Protocol fields use the same names as the
        classic web UI; choices come from the device when <code>select_options</code> is present.
      </p>
      <p class="ok">${this.msg}</p>
      <form @submit=${this.onSubmit}>
        <fieldset>
          <legend>WiFi / MQTT</legend>
          <label>SSID <input type="text" name="SSID" .value=${d.string.SSID ?? ""} /></label>
          <label>Password <input type="password" name="PASSWORD" autocomplete="new-password" placeholder="(unchanged if empty)" /></label>
          <label
            >MQTT publish interval (s)
            <input type="number" name="MQTTPUBLISHMS" min="1" step="1" .value=${String(d.mqtt_publish_s)} /></label
          >
        </fieldset>

        <fieldset>
          <legend>Protocols</legend>
          ${this.renderSelect("inverter", "Inverter protocol", sel.INVTYPE ?? 0, o?.inverter)}
          ${this.renderSelect("INVCOMM", "Inverter communication", sel.INVCOMM ?? 0, o?.comm)}
          ${this.renderSelect("battery", "Battery type", sel.BATTTYPE ?? 0, o?.battery)}
          ${this.renderSelect("BATTCHEM", "Battery chemistry", sel.BATTCHEM ?? 0, o?.chemistry)}
          ${this.renderSelect("BATTCOMM", "Battery communication", sel.BATTCOMM ?? 0, o?.comm)}
          ${this.renderSelect("charger", "Charger type", sel.CHGTYPE ?? 0, o?.charger)}
          ${this.renderSelect("CHGCOMM", "Charger communication", sel.CHGCOMM ?? 0, o?.comm)}
          ${this.renderSelect("EQSTOP", "Equipment stop button", sel.EQSTOP ?? 0, o?.eqstop)}
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
          <label
            >CT offset (string)
            <input type="text" name="CTOFFSET" .value=${d.string_extra?.CTOFFSET ?? ""} /></label
          >
        </fieldset>

        <fieldset>
          <legend>Unsigned integers</legend>
          ${uintKeys.map(
            (k) =>
              html`<label>${k}
                <input type="number" name=${k} .value=${String(d.uint[k] ?? 0)} /></label>`
          )}
        </fieldset>

        <fieldset>
          <legend>Strings</legend>
          ${strKeys.map(
            (k) =>
              html`<label>${k}
                <input type="text" name=${k} .value=${d.string[k] ?? ""} /></label>`
          )}
        </fieldset>

        <fieldset>
          <legend>Booleans</legend>
          ${boolKeys.map(
            (k) =>
              html`<label
                ><input type="checkbox" name=${k} value="on" ?checked=${d.bool[k]} />${k}</label
              >`
          )}
        </fieldset>

        <button type="submit" ?disabled=${this.busy}>Save settings</button>
        <button type="button" class="danger" @click=${this.factoryReset} ?disabled=${this.busy}>Factory reset</button>
        <button type="button" @click=${this.load} ?disabled=${this.busy}>Reload</button>
      </form>
    `;
  }
}
