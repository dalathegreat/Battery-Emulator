import { useState } from "preact/hooks";

import { upload } from "./utils/upload.tsx";

interface CanFrame {
    ts: number;
    bus: number;
    id: number;
    data: number[];
}

export interface LogInfo {
    frames: CanFrame[];
    ids: number[];
    frequencies: Record<number, number>;
}

async function parse_log(file: File): Promise<LogInfo> {
    const text = new TextDecoder().decode(await file.arrayBuffer()).toUpperCase();

    const frames: CanFrame[] = [], counts: Record<number, number> = {};
    let start_time = 0, prev_time = 0, max_ts = 0;

    text.split('\n').forEach((line) => {
        const r = line.match(/^\(([0-9.]*)\) [TR]X([0-9]*) ([A-F0-9]*) \[[0-9]*\] (.*)/);
        if(!r) return;

        const ts = parseFloat(r[1])*1000;
        // If time goes backwards, adjust start_time to keep things monotonic
        if(!start_time) start_time = ts;
        if(ts < prev_time) start_time -= (prev_time - ts);
        prev_time = ts;

        const bus = parseInt(r[2]);
        const id = parseInt(r[3], 16);
        const data = r[4].trim().split(' ').map(v => parseInt(v, 16));
        const rel = ts - start_time;
        frames.push({ ts: rel, bus, id, data });
        counts[id] = (counts[id] || 0) + 1;
        if (rel > max_ts) max_ts = rel;
    });

    const dur = max_ts / 1000, frequencies: Record<number, number> = {}, ids = Object.keys(counts).map(Number).sort((a, b) => a - b);
    if (dur > 0) ids.forEach(id => frequencies[id] = counts[id] / dur);

    return { frames, ids, frequencies };
}

function upload_log(frames: CanFrame[], selectedIds: Set<number>, iface: number, cb: (v: any) => void, loop: boolean, log: boolean, loopGap: number) {
    const list = frames.filter(f => selectedIds.has(f.id));
    if(!list.length) return alert('No frames selected'), null;

    const max_ts = list.reduce((m, f) => Math.max(m, f.ts), 0);
    const duration = (max_ts || 0) + loopGap;
    // If looping, send 5s worth of data at a time
    const repeats = loop ? Math.ceil(5000 / (duration || 1)) : 1;

    const single_size = list.reduce((i, frame) => i + 10 + frame.data.length, 0);
    const buf = new ArrayBuffer(single_size * repeats);
    const view = new DataView(buf);
    let off = 0;

    for (let i = 0; i < repeats; i++) {
        const ts_offset = i * duration;
        list.forEach(f => {
            console.log(f.ts + ts_offset);
            view.setUint32(off, f.ts + ts_offset, true); off += 4;
            view.setUint32(off, f.id, true); off += 4;
            view.setUint8(off++, f.data.length);
            view.setUint8(off++, f.bus);
            f.data.forEach(v => view.setUint8(off++, v));
        });
    }

    const avg_len = off / (list.length * repeats);
    const res = upload(import.meta.env.VITE_API_BASE + '/api/cansend?if=' + iface + "&log=" + (log?"1":"0"), buf, (progress, rate) => cb({ progress, rate: rate / avg_len }));
    res.promise.catch(([s, r]) => s && alert(`Upload failed: ${s} ${r}`));
    return res;
}


export function CanSender() {
    const [st, setSt] = useState<{progress: number, rate: number}>({ progress: -1, rate: 0 });
    const [iface, setIface] = useState(0);
    const [loop, setLoop] = useState(false);
    const [loopGap, setLoopGap] = useState(1);
    const [log, setLog] = useState(false);
    const [frames, setFrames] = useState<CanFrame[]>([]);
    const [selIds, setSelIds] = useState<Set<number>>(new Set());
    const [ids, setIds] = useState<number[]>([]);
    const [freqs, setFreqs] = useState<Record<number, number>>({});
    const [request, setRequest] = useState<{ abort: () => void } | null>(null);

    const onFile = async (ev: any) => {
        const input = ev.target as HTMLInputElement;
        if (input?.files?.[0]) {
            const d = await parse_log(input.files[0]);
            setFrames(d.frames);
            setIds(d.ids);
            setFreqs(d.frequencies);
            setSelIds(new Set(d.ids));
            input.value = '';
            setSt({ progress: -1, rate: 0 });
        }
    };

    const toggle = (id: number) => {
        const n = new Set(selIds);
        n.has(id) ? n.delete(id) : n.add(id);
        setSelIds(n);
    };

    const start = (isLoop = false, log = false) => {
        const o = upload_log(frames, selIds, iface, setSt, isLoop, log, loopGap);
        if (o) {
            setRequest(o);
            o.promise.then(() => {
                if (isLoop) start(true, log);
                else setRequest(null);
            }).catch(() => setRequest(null));
        }
    };

    const stop = () => {
        if (request) request.abort(), setRequest(null), setSt({ progress: -1, rate: 0 });
    };

    return (
        <div class="columns">
            <div>
                <h2>CAN Sender</h2>
                <p>The file should be in the format as the CAN log output.</p>
                <div class="form-row">
                    <label>CAN interface</label>
                    <select value={ iface } onChange={ (ev) => setIface(parseInt((ev.target as any).value)) }>
                        <option value="0">Native CAN</option>
                        <option value="2">MCP2515 add-on</option>
                        <option value="3">CAN FD (MCP2518 add-on)</option>
                        <option value="15">From log</option>
                    </select>
                </div>
                <div style="margin: 1.5rem 0;">
                    <input type="file" style="font-size: 1.25rem;" onChange={ onFile } />
                    <label style="margin-left: 2rem; cursor: pointer;">
                        <input type="checkbox" checked={loop} onChange={e => setLoop((e.target as any).checked)} /> Loop
                    </label>
                    <label style="margin-left: 2rem; cursor: pointer;">
                        <input type="checkbox" checked={log} onChange={e => setLog((e.target as any).checked)} /> Log
                    </label>
                    <label style="margin-left: 2rem; cursor: pointer;">
                        Loop gap (ms): <input type="number" value={loopGap} min="1" onChange={e => setLoopGap(parseInt((e.target as any).value) || 0)} style="width: 8ch; font-size: 1rem; margin-left: 0.5rem;" />
                    </label>
                </div>

                <div style="margin: 1rem 0;">
                    {request ? (
                        <button onClick={stop} style="font-size: 1.25rem; background-color: #f44; color: #fff; border: none; border-radius: 4px;">Stop</button>
                    ) : (
                        <button disabled={!frames.length || !selIds.size} onClick={() => start(loop, log)} style="font-size: 1.25rem;">Play</button>
                    )}
                </div>

                <div style="font-size: 1.25rem; margin-bottom: 5rem;"><b>
                { st.progress>=0 && <div>
                    { st.progress < 1 ? "Sending..." : "Finished" }<br />
                    Transferred: { (st.progress*100).toFixed(0) } %<br />
                    Messages buffered/sec: { (st.rate).toFixed(0) }
                </div> }
                </b></div>
            </div>
            {ids.length > 0 && (
                <div style="margin: 1rem 0 0 auto; width: 20rem; max-height: 70vh; overflow-y: auto; border: 1px solid #666; background: #212121; border-radius: 5px; padding: 1rem 1.25rem;">
                    <h3>Select IDs to send</h3>
                    <div style="display: grid; grid-template-columns: repeat(auto-fill, minmax(150px, 1fr)); gap: 0.5rem;">
                        {ids.map(id => (
                            <label key={id} style="display: flex; align-items: center; gap: 0.25rem; cursor: pointer;">
                                <input type="checkbox" checked={selIds.has(id)} onChange={() => toggle(id)} />
                                <span style="font-family: monospace;">0x{id.toString(16).toUpperCase()}</span>
                                <span style="font-size: 0.8rem; color: #aaa;">({freqs[id]?.toFixed(1)} msgs/s)</span>
                            </label>
                        ))}
                    </div>
                </div>
            )}
        </div>
    );
}
