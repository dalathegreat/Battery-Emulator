import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { api } from "../api/index.js";

@customElement("app-debuglog")
export class AppDebuglog extends LitElement {
  @state() private text = "";
  @state() private err = "";
  @state() private poll: ReturnType<typeof setInterval> | null = null;

  static styles = css`
    :host {
      display: block;
      color: #fff;
      font-family: ui-monospace, monospace;
      font-size: 12px;
    }
    .card {
      background: #303e47;
      border-radius: 12px;
      padding: 1rem;
      margin-bottom: 1rem;
    }
    pre {
      margin: 0;
      white-space: pre-wrap;
      word-break: break-word;
      max-height: 70vh;
      overflow: auto;
      background: #0d1114;
      padding: 0.75rem;
      border-radius: 8px;
    }
    button {
      background: #505e67;
      color: #fff;
      border: none;
      padding: 0.5rem 1rem;
      border-radius: 8px;
      cursor: pointer;
      margin-right: 0.5rem;
    }
    a {
      color: #9cf;
    }
    .err {
      color: #f88;
    }
  `;

  connectedCallback(): void {
    super.connectedCallback();
    void this.pull();
    this.poll = setInterval(() => void this.pull(), 2000);
  }

  disconnectedCallback(): void {
    super.disconnectedCallback();
    if (this.poll) clearInterval(this.poll);
  }

  async pull() {
    const [d, e] = await api.debuglog.get();
    if (e) {
      this.err = String(e.error_msg || e);
      return;
    }
    this.err = "";
    this.text = typeof d?.text === "string" ? d.text : "";
  }

  render() {
    return html`
      <h2>Debug log</h2>
      <div class="card">
        <p>
          Polling <code>GET /api/debug/log</code>. Export:
          <a href="/export_log" target="_blank" rel="noopener">/export_log</a>
        </p>
        <button type="button" @click=${this.pull}>Refresh now</button>
        ${this.err ? html`<p class="err">${this.err}</p>` : null}
        <pre>${this.text || "—"}</pre>
      </div>
    `;
  }
}
