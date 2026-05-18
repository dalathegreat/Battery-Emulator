import { LitElement, css, html } from "lit";
import { customElement, state } from "lit/decorators.js";
import { api } from "../api/index.js";
import { getAPIBaseURL } from "../xFetch.js";
import type { CanlogStatusResponse } from "../types/system.types.js";

@customElement("app-canlog")
export class AppCanlog extends LitElement {
  @state() private st: CanlogStatusResponse | null = null;
  @state() private msg = "";
  @state() private err = "";
  @state() private logText = "";
  @state() private cutoffInput = "";
  @state() private viewerOn = false;
  @state() private poll: ReturnType<typeof setInterval> | null = null;

  static styles = css`
    :host {
      display: block;
      color: #fff;
      font-family: system-ui, sans-serif;
    }
    .card {
      background: #303e47;
      border-radius: 12px;
      padding: 1rem;
      margin-bottom: 1rem;
    }
    button {
      background: #505e67;
      color: #fff;
      border: none;
      padding: 0.5rem 1rem;
      border-radius: 8px;
      cursor: pointer;
      margin: 0.25rem;
    }
    .err {
      color: #f88;
    }
    .ok {
      color: #8c8;
    }
    pre {
      font-family: ui-monospace, monospace;
      font-size: 11px;
      white-space: pre-wrap;
      word-break: break-word;
      max-height: 50vh;
      overflow: auto;
      background: #0d1114;
      padding: 0.75rem;
      border-radius: 8px;
    }
    label {
      display: inline-flex;
      align-items: center;
      gap: 0.35rem;
      margin: 0.35rem 0.35rem 0.35rem 0;
    }
    input[type="number"] {
      width: 6rem;
      padding: 0.25rem;
      border-radius: 6px;
      border: 1px solid #555;
      background: #1a252b;
      color: #fff;
    }
    a {
      color: #9cf;
    }
  `;

  connectedCallback(): void {
    super.connectedCallback();
    void this.load();
  }

  disconnectedCallback(): void {
    super.disconnectedCallback();
    if (this.poll) clearInterval(this.poll);
  }

  async load() {
    const [d, e] = await api.canlog.status();
    this.err = e ? String(e.error_msg || e) : "";
    this.st = e ? null : d;
  }

  async enableViewer() {
    const [, e] = await api.canlog.enableViewer();
    if (e) {
      this.msg = String(e.error_msg || e);
      return;
    }
    this.viewerOn = true;
    this.msg = "Web CAN viewer enabled.";
    if (this.poll) clearInterval(this.poll);
    void this.pullMessages();
    this.poll = setInterval(() => void this.pullMessages(), 1500);
  }

  async pullMessages() {
    const [d, e] = await api.canlog.messages();
    if (e) return;
    this.logText = d?.text ?? "";
    if (d && this.cutoffInput === "") this.cutoffInput = String(d.can_id_cutoff ?? 0);
  }

  async applyCutoff() {
    const v = parseInt(this.cutoffInput, 10);
    if (Number.isNaN(v)) return;
    const [, e] = await api.canlog.setCanIdCutoff(v);
    this.msg = e ? String(e.error_msg || e) : `Cutoff set to ${v}`;
  }

  async replayStart(loop: boolean) {
    const [t, e] = await api.canlog.startReplay(loop);
    this.msg = e ? String(e.error_msg || e) : String(t);
  }

  async replayStop() {
    const [t, e] = await api.canlog.stopReplay();
    this.msg = e ? String(e.error_msg || e) : String(t);
    await this.load();
  }

  private onFile(e: Event) {
    const inp = e.target as HTMLInputElement;
    const f = inp.files?.[0];
    if (!f) return;
    const fd = new FormData();
    fd.append("file", f, f.name);
    void (async () => {
      const res = await fetch(`${getAPIBaseURL()}/import_can_log`, {
        method: "POST",
        body: fd,
        credentials: "include",
      });
      this.msg = res.ok ? "Upload started / OK" : `Upload failed ${res.status}`;
      inp.value = "";
    })();
  }

  render() {
    return html`
      <h2>CAN log / replay</h2>
      ${this.err ? html`<p class="err">${this.err}</p>` : null}
      ${this.msg ? html`<p class="ok">${this.msg}</p>` : null}
      <button type="button" @click=${this.load}>Refresh status</button>
      ${this.st
        ? html`
            <div class="card">
              <pre>${JSON.stringify(this.st, null, 2)}</pre>
            </div>
          `
        : null}
      <div class="card">
        <h3>Replay</h3>
        <label
          >Replay interface (0–4)
          <input
            type="number"
            min="0"
            max="4"
            .value=${String(this.st?.can_replay_interface ?? 0)}
            @change=${(e: Event) => {
              const v = (e.target as HTMLInputElement).value;
              void api.canlog.setCANInterface(Number(v));
            }}
        /></label>
        <button type="button" @click=${() => this.replayStart(false)}>Start replay</button>
        <button type="button" @click=${() => this.replayStart(true)}>Start replay (loop)</button>
        <button type="button" @click=${this.replayStop}>Stop replay</button>
      </div>
      <div class="card">
        <h3>Web viewer</h3>
        <button type="button" @click=${this.enableViewer}>Enable logging + viewer</button>
        <button type="button" @click=${() => api.canlog.stopCanLogging()}>Stop CAN logging</button>
        <label
          >CAN ID cutoff <input type="number" .value=${this.cutoffInput} @input=${(e: Event) => {
            this.cutoffInput = (e.target as HTMLInputElement).value;
          }} /></label
        >
        <button type="button" @click=${this.applyCutoff}>Apply cutoff</button>
        <p>
          <a href="/export_can_log" target="_blank" rel="noopener">Export log</a> ·
          <a href="/delete_can_log" target="_blank" rel="noopener">Delete log</a>
        </p>
        <p>Import log (multipart to <code>/import_can_log</code>):</p>
        <input type="file" @change=${this.onFile} />
        ${this.viewerOn ? html`<pre>${this.logText || "—"}</pre>` : null}
      </div>
    `;
  }
}
