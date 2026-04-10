import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { api } from "../api/index.js";
import type { EventRow } from "../types/events.types.js";

@customElement("app-events")
export class AppEvents extends LitElement {
  @state() private rows: EventRow[] = [];
  @state() private err = "";

  static styles = css`
    :host {
      display: block;
      color: #fff;
      font-family: system-ui, sans-serif;
    }
    table {
      width: 100%;
      border-collapse: collapse;
      background: #303e47;
      border-radius: 8px;
      overflow: hidden;
    }
    th,
    td {
      padding: 8px;
      text-align: left;
      border-bottom: 1px solid #455a64;
      font-size: 13px;
    }
    th {
      background: #1e2c33;
    }
    .sev-ERROR {
      background: #5c1a24;
    }
    .sev-WARNING {
      background: #5c4a1a;
    }
    .sev-INFO {
      background: #1a5c3a;
    }
    button {
      background: #505e67;
      color: #fff;
      border: none;
      padding: 0.5rem 1rem;
      border-radius: 8px;
      cursor: pointer;
      margin-right: 0.5rem;
      margin-bottom: 0.5rem;
    }
    .err {
      color: #f88;
    }
  `;

  connectedCallback(): void {
    super.connectedCallback();
    this.load();
  }

  async load() {
    const [d, e] = await api.events.list();
    this.err = e ? String(e.error_msg || e) : "";
    this.rows = e || !d ? [] : d.events;
  }

  async clear() {
    if (!confirm("Clear all events?")) return;
    const [, err] = await api.events.clear();
    if (err) alert(String(err.error_msg || err));
    await this.load();
  }

  render() {
    return html`
      <h2>Events</h2>
      ${this.err ? html`<p class="err">${this.err}</p>` : null}
      <button @click=${this.load}>Refresh</button>
      <button @click=${this.clear}>Clear all</button>
      <table>
        <thead>
          <tr>
            <th>Type</th>
            <th>Severity</th>
            <th>Age (ms)</th>
            <th>Count</th>
            <th>Data</th>
            <th>Message</th>
          </tr>
        </thead>
        <tbody>
          ${this.rows.map(
            (r) => html`
              <tr class="sev-${r.severity}">
                <td>${r.type}</td>
                <td>${r.severity}</td>
                <td>${r.age_ms}</td>
                <td>${r.count}</td>
                <td>${r.data}</td>
                <td>${r.message}</td>
              </tr>
            `
          )}
        </tbody>
      </table>
    `;
  }
}
