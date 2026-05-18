import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { api } from "../api/index.js";
import type { BatteryCommandDef } from "../types/battery.types.js";
import type { BatteryExtraEntry, BatteryExtraRow } from "../types/battery-extra.types.js";

function groupExtraRows(rows: BatteryExtraRow[]) {
  const sections: { title: string; rows: BatteryExtraRow[] }[] = [];
  let title = "Extra";
  let buf: BatteryExtraRow[] = [];
  for (const r of rows) {
    if (r.label.startsWith("§ ")) {
      if (buf.length) {
        sections.push({ title, rows: buf });
        buf = [];
      }
      title = r.label.slice(2).trim() || "Extra";
    } else {
      buf.push(r);
    }
  }
  if (buf.length || sections.length === 0) {
    sections.push({ title, rows: buf });
  }
  return sections;
}

@customElement("app-advanced")
export class AppAdvanced extends LitElement {
  @state() private cmds: BatteryCommandDef[] = [];
  @state() private extra: BatteryExtraEntry[] = [];
  @state() private err = "";
  @state() private last = "";

  static styles = css`
    :host {
      display: block;
      color: #fff;
      font-family: system-ui, sans-serif;
    }
    .cmd {
      background: #303e47;
      border-radius: 10px;
      padding: 0.75rem;
      margin-bottom: 0.5rem;
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      gap: 0.5rem;
    }
    .extra-batt {
      background: #263238;
      border-radius: 10px;
      padding: 0.75rem 1rem;
      margin-bottom: 1rem;
      border: 1px solid #455a64;
    }
    .extra-batt h3 {
      margin: 0 0 0.5rem;
      font-size: 1rem;
      color: #b0bec5;
    }
    .section-title {
      margin: 0.75rem 0 0.35rem;
      font-size: 0.95rem;
      font-weight: 600;
      color: #90caf9;
      border-bottom: 1px solid #455a64;
      padding-bottom: 0.25rem;
    }
    .section-title:first-child {
      margin-top: 0;
    }
    dl {
      margin: 0;
      display: grid;
      grid-template-columns: minmax(8rem, 1fr) 2fr;
      gap: 0.25rem 0.75rem;
      font-size: 13px;
    }
    dt {
      color: #90a4ae;
      font-weight: 500;
    }
    dd {
      margin: 0;
      word-break: break-word;
    }
    button {
      background: #505e67;
      color: #fff;
      border: none;
      padding: 0.4rem 0.8rem;
      border-radius: 8px;
      cursor: pointer;
    }
    .err {
      color: #f88;
    }
    .ok {
      color: #8c8;
      font-size: 13px;
    }
    .empty {
      opacity: 0.6;
      font-size: 13px;
    }
  `;

  connectedCallback(): void {
    super.connectedCallback();
    this.load();
  }

  async load() {
    const [d, e] = await api.battery.commands();
    const [ex, e2] = await api.battery.extra();
    const parts = [e?.error_msg || e, e2?.error_msg || e2].filter(Boolean);
    this.err = parts.map(String).join(" ");
    this.cmds = e || !d ? [] : d.commands;
    this.extra = !e2 && ex?.batteries ? ex.batteries : [];
  }

  async run(cmd: BatteryCommandDef, batt: number) {
    if (cmd.confirm && !confirm(cmd.confirm)) return;
    const [, err] = await api.battery.command(cmd.id, batt);
    this.last = err
      ? `Error: ${err.error_msg || err}`
      : `OK: ${cmd.id} on battery ${batt}`;
  }

  render() {
    return html`
      <h2>Advanced battery</h2>
      ${this.err ? html`<p class="err">${this.err}</p>` : null}
      <p class="ok">${this.last}</p>
      <button @click=${this.load}>Reload commands</button>
      ${this.extra.map((b) => this.renderExtraBattery(b))}
      ${this.cmds.map(
        (c) => html`
          <div class="cmd">
            <strong>${c.label}</strong>
            <span style="opacity:.7">${c.id}</span>
            ${c.batteries.map(
              (bn) => html`
                <button @click=${() => this.run(c, bn)}>Battery ${bn}</button>
              `
            )}
          </div>
        `
      )}
    `;
  }

  private renderExtraBattery(b: BatteryExtraEntry) {
    const rows = b.rows ?? [];
    const sections = groupExtraRows(rows);
    return html`
      <div class="extra-batt">
        <h3>Battery ${b.index} · extra status</h3>
        ${rows.length === 0
          ? html`<p class="empty">(no rows)</p>`
          : sections.map(
              (s) => html`
                <div class="section">
                  <div class="section-title">${s.title}</div>
                  ${s.rows.length === 0
                    ? html`<p class="empty">(empty)</p>`
                    : html`<dl>
                        ${s.rows.map(
                          (r) => html`
                            <dt>${r.label}</dt>
                            <dd>${r.value}</dd>
                          `
                        )}
                      </dl>`}
                </div>
              `
            )}
      </div>
    `;
  }
}
