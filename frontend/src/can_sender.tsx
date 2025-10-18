import { useState } from "preact/hooks";

import { upload } from "./utils/upload.tsx";

async function transmit_log(file: File, iface: number, cb: (v: any) => void) {
    const buf = await file.arrayBuffer();
    const text = new TextDecoder().decode(buf).toUpperCase();

    const send_buf = new ArrayBuffer(buf.byteLength);
    const view = new DataView(send_buf);
    let offset = 0;
    let start_time = 0;
    let prev_time = 0;
    text.split('\n').forEach((line) => {
        const r = line.match(/^\(([0-9]+(?:\.[0-9]+)?)\) ([TR]X)[0-9]+ ([A-F0-9]+) \[[0-9]+\] ((?:[A-F0-9]+ ?)*)/);
        if(!r) return;

        const ts = parseFloat(r[1])*1000;
        if(start_time===0) start_time = ts;
        // If time goes backwards, adjust start_time to keep things monotonic
        if(ts < prev_time) start_time -= (prev_time - ts);
        prev_time = ts;

        // Timestamp in ms
        view.setUint32(offset, ts - start_time, true); offset += 4;
        // Address
        view.setUint32(offset, parseInt(r[3], 16), true); offset += 4;
        const data = r[4].trim().split(' ').map((v) => parseInt(v, 16));
        // Data length
        view.setUint8(offset, data.length); offset += 1;
        // Data
        data.forEach((v) => { view.setUint8(offset, v); offset += 1; });
    });
    if(offset === 0) {
        alert('No valid CAN frames found in the file.');
        return;
    }
    const avg_len = offset / text.split('\n').length;
    const final_buf = send_buf.slice(0, offset);
    upload(import.meta.env.VITE_API_BASE + '/api/cansend?if=' + iface, final_buf, (progress, rate) => cb({
        progress,
        // Convert bytes/sec to (approx) messages/sec
        rate: rate / avg_len,
    })).catch(([status, response]) => {
        alert('Upload failed: ' + status + ' ' + response);
    });
}


export function CanSender() {
    const [status, setStatus] = useState<{progress: number, rate: number}>({ progress: -1, rate: 0 });
    const [iface, setIface] = useState(0);

    return (
        <div>
            <h2>CAN Sender</h2>
            <p>The file should be in the format as the CAN log output.</p>
            <div class="form-row">
                <label>CAN interface</label>
                <select value={ iface } onChange={ (ev) => { setIface(parseInt((ev.target as HTMLSelectElement).value)); } }>
                    <option value="0">Native CAN</option>
                    <option value="2">MCP2515 add-on</option>
                    <option value="3">CAN FD (MCP2518 add-on)</option>
                </select>
            </div>
            <div style="margin: 1.5rem 0;">
            <input type="file" style="font-size: 1.25rem; border: 2px solid #333; border-radius: 0.5rem;" onChange={ (ev) => {
                const input = ev.target as HTMLInputElement;
                if (input?.files?.length) {
                    transmit_log(input.files[0], iface, setStatus);
                    input.value = '';
                }
            } } />
            </div>
            <div style="margin: 0 0 5rem; font-size: 1.25rem"><b>
            { status.progress>=0 ? <div>
                Transferred: { (status.progress*100).toFixed(0) } %<br />
                Messages buffered/sec: { (status.rate).toFixed(0) }
            </div> : null }
            </b></div>
        </div>
    );
}
