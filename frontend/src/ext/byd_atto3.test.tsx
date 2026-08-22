// Browserless tests for src/ext/byd_atto3.tsx, run with `bun test`.
//
// The component reads /api/batext (a binary DATALAYER_INFO_BYDATTO3 blob)
// through the generated offset table and writes settings through
// src/utils/api.tsx; that module is mocked (mock.module) so the test runs
// against a crafted DataView and a recorded apiPost.
import { describe, test, expect, beforeEach, mock } from "bun:test";
import { render, screen, fireEvent, waitFor, cleanup } from "@testing-library/preact";

import { DATALAYER_INFO_BYDATTO3_FIELDS } from "./datalayer/DATALAYER_INFO_BYDATTO3.ts";
import { LENGTHS } from "./consts.ts";

const apiPost = mock(async (_url: string, _data: any) => ({}));
const refreshApi = mock(() => {});

// Same specifier resolution as byd_atto3.tsx's `../utils/api.tsx`.
mock.module("../utils/api.tsx", () => ({
    apiPost,
    refreshApi,
}));

const { BydAtto3Extended } = await import("./byd_atto3.tsx");

// Build the batext payload the same way the device does: u32 battery-type
// header + struct bytes, fields at the offsets in the generated table.
function buildView(overrides: Record<string, number> = {}): DataView {
    let total = 4;
    for (const [, type, arr] of DATALAYER_INFO_BYDATTO3_FIELDS) {
        total += LENGTHS[type] * (arr || 1);
    }
    const buf = new ArrayBuffer(total);
    const dv = new DataView(buf);
    dv.setUint32(0, 5, true); // BatteryType::BydAtto3
    let offset = 4;
    for (const [name, type, arr] of DATALAYER_INFO_BYDATTO3_FIELDS) {
        const len = LENGTHS[type];
        const n = arr || 1;
        for (let i = 0; i < n; i++) {
            const v = overrides[name] ?? 0;
            switch (type) {
                case 'u8': case 'b': dv.setUint8(offset, v); break;
                case 'u16': dv.setUint16(offset, v, true); break;
                case 'u32': dv.setUint32(offset, v, true); break;
                case 'i16': dv.setInt16(offset, v, true); break;
                case 'f': dv.setFloat32(offset, v, true); break;
            }
            offset += len;
        }
    }
    return dv;
}

const ATTO3 = {
    SOC_highprec: 423,                    // 42.3 %
    pack_voltage_dV: 4125,                // 412.5 V
    contactor_control_state: 1,           // "Closed (live)"
    contactor_main_closed: 1,
    contactor_feedback: 0x84,             // "Closed idle, HV active"
    dischargePower: 300,                  // 30.0 kW
    chargePower: 150,                     // 15.0 kW
    battery_temperatures: 25,
    auto_calibrate_soc_enabled: 1,
    auto_calibrate_soc_drift_percent: 5,
    calibrationTargetSOC: 100,
    calibrationTargetAH: 150,
};

beforeEach(() => {
    cleanup();
    apiPost.mockClear();
    refreshApi.mockClear();
});

describe("BydAtto3Extended rendering", () => {
    test("renders scaled values from the batext blob", () => {
        render(<BydAtto3Extended view={buildView(ATTO3)} />);

        expect(screen.getByText("SOC measured: 42.3%")).toBeTruthy();
        expect(screen.getByText("Pack voltage: 412.5 V")).toBeTruthy();
        expect(screen.getByText(/BE contactor state: Closed \(live\)/)).toBeTruthy();
        expect(screen.getByText(/BMS pack mode: Closed idle, HV active/)).toBeTruthy();
        expect(screen.getByText("Max discharge power: 30.0 kW")).toBeTruthy();
        expect(screen.getByText("Max charge (regen) power: 15.0 kW")).toBeTruthy();
        expect(screen.getByText(/Temperature sensor 1: 25 °C/)).toBeTruthy();
    });

    test("skips the 215 °C temperature sentinel", () => {
        const { container } = render(<BydAtto3Extended view={buildView({ ...ATTO3, battery_temperatures: 215 })} cells={3} />);
        expect(screen.getByText("Detected cells: 3")).toBeTruthy();

        // All 13 sensors at 215 must render no temperature rows at all
        const rows = container.querySelectorAll('h4');
        for (const h of rows) {
            expect(h.textContent).not.toMatch(/Temperature sensor/);
        }
        // ...but other rows still appear
        expect(screen.getByText(/Seed: 0 SolvedKey: 0/)).toBeTruthy();
    });

    test("auto-calibration controls reflect the blob and persist changes", async () => {
        const { container } = render(<BydAtto3Extended view={buildView(ATTO3)} />);

        // Enabled checkbox ticked (auto_calibrate_soc_enabled = 1)
        const enabled = container.querySelector('input[type="checkbox"]') as HTMLInputElement;
        expect(enabled.checked).toBe(true);

        // Drift input seeded from the blob
        const drift = container.querySelector('input[type="number"]') as HTMLInputElement;
        expect(drift.value).toBe("5");

        // Toggling auto-calibrate writes BYDAUTOCALEN through the settings API
        fireEvent.click(enabled);
        expect(apiPost).toHaveBeenCalledWith("/api/internal/settings", { BYDAUTOCALEN: "0" });
        // refreshApi runs after the (async) settings POST resolves
        await waitFor(() => expect(refreshApi).toHaveBeenCalled());
    });

    test("saving a drift change writes BYDAUTOCALDRIFT", () => {
        const { container } = render(<BydAtto3Extended view={buildView(ATTO3)} />);

        const drift = container.querySelector('input[type="number"]') as HTMLInputElement;
        drift.value = "8";
        fireEvent.input(drift);

        apiPost.mockClear();
        fireEvent.click(screen.getByRole("button", { name: "Save Drift %" }));
        expect(apiPost).toHaveBeenCalledWith("/api/internal/settings", { BYDAUTOCALDRIFT: "8" });
    });

    test("battery 2 uses the second-battery settings keys", () => {
        const { container } = render(<BydAtto3Extended view={buildView(ATTO3)} battery={2} />);

        const enabled = container.querySelector('input[type="checkbox"]') as HTMLInputElement;
        fireEvent.click(enabled);
        expect(apiPost).toHaveBeenCalledWith("/api/internal/settings", { BYDAUTOCALEN2: "0" });
    });
});
