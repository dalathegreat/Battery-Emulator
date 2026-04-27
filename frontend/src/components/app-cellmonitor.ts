import { LitElement, css, html } from "lit";
import { styleMap } from "lit/directives/style-map.js";
import { customElement, state } from "lit/decorators.js";
import { api } from "../api/index.js";
import type { BatteryCellGroup } from "../types/battery.types.js";

@customElement("app-cellmonitor")
export class AppCellmonitor extends LitElement {
  @state() private packs: [BatteryCellGroup, BatteryCellGroup, BatteryCellGroup] | null =
    null;
  @state() private err = "";
  private timer: ReturnType<typeof setInterval> | null = null;

  static styles = css`
    :host {
      display: block;
      color: #fff;
      font-family: system-ui, sans-serif;
    }
    .pack {
      background: #303e47;
      border-radius: 12px;
      padding: 1rem;
      margin-bottom: 1rem;
    }
    .cells {
      display: flex;
      flex-wrap: wrap;
      gap: 4px;
      margin-top: 0.5rem;
    }
    .cell {
      width: 36px;
      height: 48px;
      background: linear-gradient(to top, #4caf50 0%, #1e2c33 100%);
      border-radius: 4px;
      font-size: 9px;
      display: flex;
      align-items: flex-end;
      justify-content: center;
      padding: 2px;
      box-sizing: border-box;
      border: 1px solid #455a64;
    }
    .cell.bal {
      box-shadow: 0 0 6px #ff9800;
    }
    .cell.low {
      border-color: #f44336;
    }
    button {
      background: #505e67;
      color: #fff;
      border: none;
      padding: 0.5rem 1rem;
      border-radius: 8px;
      cursor: pointer;
    }
    .err {
      color: #f88;
    }
  `;

  connectedCallback(): void {
    super.connectedCallback();
    this.load();
    this.timer = setInterval(() => this.load(), 5000);
  }

  disconnectedCallback(): void {
    super.disconnectedCallback();
    if (this.timer) clearInterval(this.timer);
  }

  async load() {
    const [data, err] = await api.battery.cellmonitor();
    if (err) {
      this.err = String(err.error_msg || err);
      return;
    }
    this.err = "";
    this.packs = data.packs;
  }

  renderPack(title: string, pack: BatteryCellGroup) {
    if (!pack.present || pack.cells.length === 0) {
      return html`<div class="pack"><h3>${title}</h3><p>No cells</p></div>`;
    }
    const maxV = Math.max(...pack.cells.map((c) => c.voltage_mV));
    const minV = Math.min(...pack.cells.map((c) => c.voltage_mV));
    const span = Math.max(1, maxV - minV);
    return html`
      <div class="pack">
        <h3>${title} (${pack.number_of_cells} cells)</h3>
        <div class="cells">
          ${pack.cells.map((c, i) => {
            const pct = ((c.voltage_mV - minV) / span) * 100;
            return html`
              <div
                class="cell ${c.balancing ? "bal" : ""} ${c.voltage_mV < 3000 ? "low" : ""}"
                title="Cell ${i + 1}: ${c.voltage_mV} mV"
                style=${styleMap({
                  background: `linear-gradient(to top,#4caf50 ${pct}%,#1e2c33 0%)`,
                })}
              >
                ${(c.voltage_mV / 1000).toFixed(2)}
              </div>
            `;
          })}
        </div>
      </div>
    `;
  }

  render() {
    if (!this.packs && !this.err) return html`<p>Loading…</p>`;
    if (this.err) return html`<p class="err">${this.err}</p><button @click=${this.load}>Retry</button>`;
    if (!this.packs) return html``;
    return html`
      <h2>Cell monitor</h2>
      <button @click=${this.load}>Refresh</button>
      ${this.renderPack("Pack 1", this.packs[0])}
      ${this.renderPack("Pack 2", this.packs[1])}
      ${this.renderPack("Pack 3", this.packs[2])}
    `;
  }
}
