// Tests for src/utils/reboot.tsx — the reboot + uptime-rollback detection.
//
// reboot() POSTs to /api/reboot, receives the pre-reboot uptime, then polls
// /api/status until the reported uptime drops below that threshold (meaning
// the device has actually restarted).  On success it redirects to "../".
//
// These tests use jest.useFakeTimers() to drive the 1-second poll loop
// deterministically and mock fetch() to return controlled responses.

import { describe, test, expect, beforeEach, afterEach, jest, mock } from "bun:test";

// ── Mock setup ──────────────────────────────────────────────────────────────

const API_BASE = "http://test-device";

// Pre-populate the env var so reboot.tsx picks it up.
try {
    (import.meta as any).env = { ...(import.meta as any).env, VITE_API_BASE: API_BASE };
} catch { /* import.meta may be sealed */ }

let fetchMock: ReturnType<typeof mock>;
let originalHref: string;

/** Set up a queue of responses for fetch(). */
function setupFetchQueue(responses: Array<object | Error>) {
    let idx = 0;
    fetchMock.mockImplementation((..._args: any[]) => {
        const i = idx++;
        if (i >= responses.length) {
            return Promise.reject(new Error(`unexpected fetch call #${i}`));
        }
        const resp = responses[i];
        if (resp instanceof Error) {
            return Promise.reject(resp);
        }
        return Promise.resolve({
            json: () => Promise.resolve(resp),
            headers: { get: () => "" },
        });
    });
}

/** Flush microtasks so promise .then() callbacks settle. */
async function flush() {
    for (let i = 0; i < 5; i++) {
        await Promise.resolve();
    }
}

async function getReboot() {
    const mod = await import("./utils/reboot.tsx");
    return mod.reboot;
}

beforeEach(() => {
    // mock() from bun:test creates a mock function (like jest.fn()).
    fetchMock = mock(() => Promise.reject(new Error("fetch not configured")));
    (globalThis as any).fetch = fetchMock;

    originalHref = window.location.href;
    (window as any)._rebooting = false;

    jest.useFakeTimers();
});

afterEach(() => {
    jest.useRealTimers();
});

// ── Tests ───────────────────────────────────────────────────────────────────

describe("reboot", () => {
    test("sets window._rebooting to true immediately", async () => {
        const reboot = await getReboot();
        setupFetchQueue([{ uptime: 100 }]);

        reboot();

        expect((window as any)._rebooting).toBe(true);
    });

    test("POSTs to /api/reboot with method POST", async () => {
        const reboot = await getReboot();
        setupFetchQueue([{ uptime: 1234 }]);

        reboot();
        await flush();

        expect(fetchMock).toHaveBeenCalledTimes(1);
        const [url, opts] = fetchMock.mock.calls[0];
        expect(url).toContain("/api/reboot");
        expect(opts).toEqual({ method: "POST" });
    });

    test("starts polling /api/status after 1-second initial delay", async () => {
        const reboot = await getReboot();
        setupFetchQueue([
            { uptime: 100 }, // POST reboot
            { uptime: 50 },  // first poll
        ]);

        reboot();
        await flush();

        // Only the POST has been made so far.
        expect(fetchMock).toHaveBeenCalledTimes(1);

        // Advance 0.5s — poll not yet triggered.
        jest.advanceTimersByTime(500);
        await flush();
        expect(fetchMock).toHaveBeenCalledTimes(1);

        // Advance to 1s total — first poll fires.
        jest.advanceTimersByTime(500);
        await flush();
        expect(fetchMock).toHaveBeenCalledTimes(2);

        // The poll URL should be /api/status.
        const pollUrl = fetchMock.mock.calls[1][0];
        expect(pollUrl).toContain("/api/status");
    });

    test("continues polling while uptime >= pre-reboot uptime", async () => {
        const reboot = await getReboot();
        setupFetchQueue([
            { uptime: 300 }, // POST reboot
            { uptime: 400 }, // poll 1
            { uptime: 500 }, // poll 2
            { uptime: 300 }, // poll 3 (equal, not less)
            { uptime: 300 }, // poll 4 (loop continues)
        ]);

        reboot();
        await flush();

        // Poll 1 at t=1s
        jest.advanceTimersByTime(1000);
        await flush();
        expect(fetchMock).toHaveBeenCalledTimes(2);

        // Poll 2 at t=2s
        jest.advanceTimersByTime(1000);
        await flush();
        expect(fetchMock).toHaveBeenCalledTimes(3);

        // Poll 3 at t=3s (uptime equals pre-reboot, still no redirect)
        jest.advanceTimersByTime(1000);
        await flush();
        expect(fetchMock).toHaveBeenCalledTimes(4);

        // Poll 4 at t=4s — uptime still equals pre-reboot value (300).
        // The check `uptime < preRebootUptime` (300 < 300) is false,
        // so the loop continues (would schedule another setTimeout).
        jest.advanceTimersByTime(1000);
        await flush();
        expect(fetchMock).toHaveBeenCalledTimes(5);
    });

    test("redirects to ../ when uptime drops below pre-reboot value", async () => {
        const reboot = await getReboot();
        setupFetchQueue([
            { uptime: 500 }, // POST reboot
            { uptime: 600 }, // poll 1 (still high)
            { uptime: 42 },  // poll 2 (rolled back!)
        ]);

        reboot();
        await flush();

        // Poll 1 at t=1s
        jest.advanceTimersByTime(1000);
        await flush();
        expect(fetchMock).toHaveBeenCalledTimes(2);

        // Poll 2 at t=2s — uptime 42 < 500 → should redirect and stop polling.
        jest.advanceTimersByTime(1000);
        await flush();
        expect(fetchMock).toHaveBeenCalledTimes(3);

        // No more polls should be scheduled.
        jest.advanceTimersByTime(5000);
        await flush();
        expect(fetchMock).toHaveBeenCalledTimes(3);
    });

    test("retries on fetch errors (device still booting)", async () => {
        const reboot = await getReboot();
        setupFetchQueue([
            { uptime: 100 },         // POST reboot
            new Error("conn refused"), // poll 1 (device down)
            new Error("conn refused"), // poll 2 (still down)
            { uptime: 5 },           // poll 3 (back online, rolled back)
        ]);

        reboot();
        await flush();

        // Poll 1 — fails
        jest.advanceTimersByTime(1000);
        await flush();
        expect(fetchMock).toHaveBeenCalledTimes(2);

        // Poll 2 — fails again
        jest.advanceTimersByTime(1000);
        await flush();
        expect(fetchMock).toHaveBeenCalledTimes(3);

        // Poll 3 — succeeds, uptime 5 < 100
        jest.advanceTimersByTime(1000);
        await flush();
        expect(fetchMock).toHaveBeenCalledTimes(4);

        // Loop should stop now.
        jest.advanceTimersByTime(5000);
        await flush();
        expect(fetchMock).toHaveBeenCalledTimes(4);
    });

    test("polls with AbortSignal", async () => {
        const reboot = await getReboot();
        setupFetchQueue([
            { uptime: 100 },
            { uptime: 50 },
        ]);

        reboot();
        await flush();
        
        // Only POST so far
        expect(fetchMock).toHaveBeenCalledTimes(1);

        // Advance 1s to trigger first poll
        jest.advanceTimersByTime(1000);
        await flush();

        // The poll call should include a signal option.
        expect(fetchMock).toHaveBeenCalledTimes(2);
        const pollOpts = fetchMock.mock.calls[1][1];
        expect(pollOpts).toBeDefined();
        expect(pollOpts.signal).toBeDefined();
    });

    test("returns a promise that never resolves", async () => {
        const reboot = await getReboot();
        setupFetchQueue([{ uptime: 100 }]);

        const p = reboot();
        await flush();

        let resolved = false;
        p.then(() => { resolved = true; });

        jest.advanceTimersByTime(5000);
        await flush();

        expect(resolved).toBe(false);
    });

    test("edge case: uptime equals pre-reboot value is not treated as rollback", async () => {
        const reboot = await getReboot();
        setupFetchQueue([
            { uptime: 500 }, // POST reboot
            { uptime: 500 }, // poll 1 — exactly equal
            { uptime: 500 }, // poll 2 — still equal
            { uptime: 499 }, // poll 3 — finally less
        ]);

        reboot();
        await flush();

        for (let i = 0; i < 3; i++) {
            jest.advanceTimersByTime(1000);
            await flush();
        }

        // After 3 polls, all returned uptime >= 500.
        expect(fetchMock).toHaveBeenCalledTimes(4); // POST + 3 polls

        // Final poll with uptime 499 < 500 triggers redirect and stops.
        jest.advanceTimersByTime(1000);
        await flush();
        expect(fetchMock).toHaveBeenCalledTimes(4); // no 5th poll

        jest.advanceTimersByTime(5000);
        await flush();
        expect(fetchMock).toHaveBeenCalledTimes(4);
    });
});
