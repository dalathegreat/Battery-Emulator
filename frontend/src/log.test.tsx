// Tests for src/log.tsx — the realtime web-log view.
//
// The component polls GET /api/log (once per second), keeping a byte position
// and asking for only the new bytes since then. The device responds with a
// binary header (pos u64 LE) followed by raw log text. The position only ever
// increases, so the view also flags gaps (bytes skipped) and reboots (position
// went backwards).
//
// These tests drive the poll loop with jest fake timers and serve controlled
// binary responses through a mocked fetch().

import { describe, test, expect, beforeEach, afterEach, jest, mock } from "bun:test";
import { render, screen, cleanup, fireEvent } from "@testing-library/preact";

let fetchMock: ReturnType<typeof mock>;
let calls: string[] = [];

// Encode a log response with the given logical position and UTF-8 text into a
// Response object. The first 8 bytes are the position (u64 LE), the rest is
// raw UTF-8 text.
function logResponse(pos: number, chunk: string): Response {
    const bytes = new TextEncoder().encode(chunk);
    const buf = new ArrayBuffer(8 + bytes.length);
    const dv = new DataView(buf);
    dv.setBigUint64(0, BigInt(pos), true);
    new Uint8Array(buf, 8).set(bytes);
    return new Response(buf, { headers: { 'Content-Type': 'application/octet-stream' } });
}

function queueLog(...responses: Array<Response | Error>) {
    let idx = 0;
    fetchMock.mockImplementation((url: any, _init?: any) => {
        calls.push(String(url));
        const i = idx++;
        if (i >= responses.length) {
            return Promise.reject(new Error(`unexpected fetch call #${i}: ${url}`));
        }
        const resp = responses[i];
        return resp instanceof Error ? Promise.reject(resp) : Promise.resolve(resp);
    });
}

async function flush() {
    for (let i = 0; i < 8; i++) {
        await Promise.resolve();
    }
}

async function getLog() {
    const { Log } = await import('./log.tsx');
    return Log;
}

beforeEach(() => {
    jest.useFakeTimers();
    calls = [];
    fetchMock = mock(() => Promise.reject(new Error('no fetch mock')));
    // eslint-disable-next-line no-global-assign
    (globalThis as any).fetch = fetchMock;
});

afterEach(() => {
    cleanup();
    jest.useRealTimers();
});

describe("Log", () => {
    test("loads the full history first, then polls for new bytes only", async () => {
        const Log = await getLog();
        queueLog(
            logResponse(14, "line one\nline two\n"),              // initial: no pos
            logResponse(25, "line three\n"),                      // ?pos=14 (14+11 bytes)
            logResponse(25, ""),                                  // nothing new
        );
        render(<Log />);

        await flush();
        expect(calls[0]).toBe('/api/log?pos=0');                  // first request starts at 0 (server treats it as "everything")
        expect(screen.getByText(/line one/).textContent).toContain('line two');

        jest.advanceTimersByTime(1000);
        await flush();
        expect(calls[1]).toBe('/api/log?pos=14');                 // subsequent requests ask only for new bytes
        expect(screen.getByText(/line three/)).toBeTruthy();

        jest.advanceTimersByTime(1000);
        await flush();
        expect(calls[2]).toBe('/api/log?pos=25');
    });

    test("renders a gap marker when bytes were skipped", async () => {
        const Log = await getLog();
        queueLog(
            logResponse(14, "line one\n"),
            logResponse(30, "line two\n"),                 // 7 bytes never delivered (30-14-9)
        );
        render(<Log />);
        await flush();
        jest.advanceTimersByTime(1000);
        await flush();

        const el = screen.getByText(/skipped/);
        expect(el.textContent).toContain('7');
        expect(screen.getByText(/line two/)).toBeTruthy();
    });

    test("renders a reboot marker when the device restarted its log", async () => {
        const Log = await getLog();
        queueLog(
            logResponse(14, "line one\n"),
            logResponse(5, "fresh boot line\n"),          // pos went backwards -> reboot
        );
        render(<Log />);
        await flush();
        jest.advanceTimersByTime(1000);
        await flush();

        expect(screen.getByText(/reboot/)).toBeTruthy();
        expect(screen.getByText(/fresh boot line/)).toBeTruthy();
    });

    test("keeps polling and appending even while scrolled up", async () => {
        const Log = await getLog();
        queueLog(
            logResponse(14, "line one\n"),
            logResponse(23, "line two\n"),
        );
        render(<Log />);
        await flush();

        const el = document.querySelector('.log') as HTMLElement;
        // If we scroll up, autoscrolling should stop.
        Object.defineProperty(el, 'scrollHeight', { configurable: true, value: 1000 });
        Object.defineProperty(el, 'clientHeight', { configurable: true, value: 100 });
        el.scrollTop = 100;                             // 1000 - 100 - 100 = 800px from the bottom
        fireEvent.scroll(el);

        jest.advanceTimersByTime(1000);                 // new line arrives anyway
        await flush();
        expect(calls.length).toBe(2);
        expect(calls[1]).toBe('/api/log?pos=14');
        expect(screen.getByText(/line two/)).toBeTruthy();
        expect(el.childNodes.length).toBe(2);           // appended as new nodes, never replaced
        expect(el.scrollTop).toBe(100);                 // view not yanked to the bottom
    });

    test("survives a transient network error and retries", async () => {
        const Log = await getLog();
        queueLog(
            logResponse(14, "line one\n"),
            new Error("connection reset"),                         // transient failure
            logResponse(23, "line two\n"),
        );
        render(<Log />);
        await flush();
        jest.advanceTimersByTime(1000);
        await flush();
        expect(screen.getByText(/line one/)).toBeTruthy();         // history intact

        jest.advanceTimersByTime(1000);
        await flush();
        expect(calls[2]).toBe('/api/log?pos=14');                  // retried from same position
        expect(screen.getByText(/line two/)).toBeTruthy();
    });
});
