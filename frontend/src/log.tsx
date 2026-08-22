import { useEffect, useRef } from 'preact/hooks'

// Display the web server log with realtime updates.

// The log is a ring buffer, which we can retrieve in chunks via GET
// /api/log?pos=NNN. Each chunk has a uint64 LE position followed by raw UTF-8
// text. We send the position when requesting the next chunk to get the new
// bytes since. If the position ever goes backwards, the device must have
// rebooted.

const HEADER_SIZE = 8;
const POLL_MS = 1000;

export function Log() {
    const view = useRef<HTMLDivElement>(null);
    const stick = useRef(true);               // whether we're at the bottom (and should autoscroll)
    const pos = useRef<number | null>(null);  // last byte position received (null = first fetch)
    const decoder = useRef(new TextDecoder('utf-8'));

    useEffect(() => {
        let cancelled = false;
        let timer: number | undefined;

        const poll = async () => {
            const base = import.meta.env.VITE_API_BASE || '';
            const url = base + '/api/log?pos=' + (pos.current ?? 0);
            try {
                const resp = await fetch(url);
                const bytes = new Uint8Array(await resp.arrayBuffer());
                if (cancelled || bytes.length < HEADER_SIZE) return;

                const dv = new DataView(bytes.buffer, bytes.byteOffset, bytes.length);
                const prev = pos.current;
                const next = Number(dv.getBigUint64(0, true));
                let chunk = decoder.current.decode(bytes.subarray(HEADER_SIZE), { stream: true });

                const missed = prev !== null ? next - prev - (bytes.length - HEADER_SIZE) : 0;
                pos.current = next;
                // Positive missed means we haven't read fast enough, and have missed data
                if (missed > 0) chunk = `----- ${missed.toLocaleString()} bytes skipped -----\n` + chunk;
                // Negative missed means the device has rebooted since our last poll
                if (missed < 0) chunk = '----- reboot -----\n' + chunk;

                const el = view.current;
                if (el) {
                    el.append(chunk);
                    if (stick.current) el.scrollTop = el.scrollHeight;
                }
            } catch {
                // Ignore, we'll retry on the next poll.
            } finally {
                if (!cancelled) timer = setTimeout(poll, POLL_MS) as unknown as number;
            }
        };

        poll();
        return () => { cancelled = true; clearTimeout(timer); };
    }, []);

    const onScroll = () => {
        const el = view.current;
        if (el) stick.current = el.scrollHeight - el.scrollTop - el.clientHeight < 40;
    };

    return (
        <>
            <h2>Log</h2>
            <div class="log" ref={view} onScroll={onScroll} />
        </>
    );
}
