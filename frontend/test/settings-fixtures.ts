// Test fixtures + a controllable in-memory backend for the settings page.
//
// settings.tsx talks to the device through src/utils/api.tsx (useGetApi /
// apiPost / refreshApi). Tests replace that module via bun's mock.module()
// with the MockBackend below, which:
//   * serves the settings/status payloads with real battery/inverter option
//     lists (the form selects are keyed by array index),
//   * applies apiPost() writes as a real backend would (mutating the payload
//     and flipping reboot_required),
//   * lets refreshApi() re-deliver fresh payloads so useGetApi re-renders -
//     exercising the page's post-save refresh path.
//
// Uses the same preact hooks instance as the app (single module graph), so
// the hook state machinery lines up.
import { useState, useEffect } from "preact/hooks";
import { jest } from "bun:test";

export const BATTERIES: (string | null)[] = [
    "None",
    null,
    "BMW i3",
    "BMW iX and i4-7 platform",
    "Chevrolet Bolt EV/Opel Ampera-e",
    "BYD Atto 3/Seal/Dolphin",
    "Cellpower BMS",
    "Chademo V2X mode",
    "CMFA platform, 27 kWh battery",
    "FoxESS HV2600/ECS4100 OEM battery",
    "Geely Geometry C",
    "DIY battery with Orion BMS (Victron setting)",
    "Sono Motors Sion 64kWh LFP ",
    "Stellantis ECMP battery",
    "I-Miev / C-Zero / Ion Triplet",
    "Jaguar I-PACE",
    "Kia/Hyundai EGMP platform",
    "Kia/Hyundai 64/40kWh battery",
    "Kia/Hyundai Hybrid",
    "Volkswagen Group MEB platform via CAN-FD",
    "MG 5 battery",
    "Nissan LEAF battery",
    "Pylon compatible battery",
    "DALY RS485",
    "RJXZS BMS, DIY battery",
    "Range Rover 13kWh PHEV battery (L494/L405)",
    "Renault Kangoo",
    "Renault Twizy",
    "Renault Zoe Gen1 22/40kWh",
    "Renault Zoe Gen2 50kWh",
    "Santa Fe PHEV",
    "SIMPBMS battery",
    "Tesla Model 3/Y",
    "Tesla Model S/X",
    "Fake battery for testing purposes",
    "Volvo / Polestar 69/78kWh SPA battery",
    "Volvo PHEV battery",
    "MG HS PHEV 16.6kWh battery",
    "Samsung SDI LV Battery",
    "Hyundai Ioniq Electric 28kWh",
    "Kia 64kWh FD battery",
    "Relion LV protocol via 250kbps CAN",
    "Rivian R1T large 135kWh battery",
    "BMW PHEV Battery",
    "Ford Mustang Mach-E battery",
    "Stellantis CMP Smart Car Battery",
    null,
    "Think City",
    "Tesla Model S/X 2012-2020",
    "Growatt HV ARK battery (battery-facing CAN)",
    "Volvo/Zeekr/Geely SEA battery",
    "Thunderstruck BMS",
    "ENNOID BMS via VESC, DIY battery",
];

export const INVERTERS: (string | null)[] = [
    "None",
    "Afore battery over CAN",
    "BYD Battery-Box Premium HVS over CAN Bus",
    "BYD 11kWh HVM battery over Modbus RTU",
    "Ferroamp Pylon battery over CAN bus",
    "FoxESS compatible HV2600/ECS4100 battery",
    "Growatt High Voltage protocol via CAN",
    "Growatt Low Voltage (48V) protocol via CAN",
    "Growatt WIT compatible battery via CAN",
    "BYD battery via Kostal RS485",
    "Pylontech HV battery over CAN bus",
    "Pylontech LV battery over CAN bus",
    "Schneider V2 SE BMS CAN",
    null,
    "SMA compatible BYD Battery-Box H",
    "SMA Low Voltage (48V) protocol via CAN",
    "SMA compatible BYD Battery-Box HVS",
    "Sofar BMS (Extended) via CAN, Battery ID",
    "SolaX Triple Power LFP over CAN bus",
    "Solxpow compatible battery",
    "Sol-Ark LV protocol over CAN bus",
    "Sungrow SBRXXX emulation over CAN bus",
    "VCU mode: Nissan LEAF battery",
    "Pylon low voltage via RS485",
];

export interface SettingsPayload {
    batteries: (string | null)[];
    inverters: (string | null)[];
    settings: Record<string, string>;
    reboot_required: boolean;
}

export interface StatusPayload {
    ip: string;
    gateway: string;
    subnet: string;
    dns: string;
}

export function makeSettingsPayload(
    settings: Record<string, string>,
    reboot_required = false,
): SettingsPayload {
    return {
        batteries: BATTERIES,
        inverters: INVERTERS,
        settings: { ...settings },
        reboot_required,
    };
}

export function makeStatusPayload(status: Partial<StatusPayload> = {}): StatusPayload {
    const base: StatusPayload = {
        ip: "192.168.1.55",
        gateway: "192.168.1.1",
        subnet: "255.255.255.0",
        dns: "8.8.8.8",
    };
    return { ...base, ...status };
}

/**
 * In-memory stand-in for /api/internal/settings + /api/status. Swap it into
 * the api.tsx module with mock.module(); see settings.test.tsx.
 */
export class MockBackend {
    settings: SettingsPayload;
    status: StatusPayload;

    /** Subscribe a useGetApi instance; notified on POST writes. */
    private listeners = new Set<() => void>();

    /** mocks of the api.tsx surface, callable/assertable by tests */
    readonly apiPost = jest.fn(async (_url: string, data: Record<string, string>) => {
        this.settings = makeSettingsPayload(
            { ...this.settings.settings, ...data },
            true, // the real backend wants a reboot after any setting change
        );
        this.notify();
        return { ok: true };
    });
    readonly refreshApi = jest.fn(() => {
        this.notify();
    });

    constructor(settings?: Record<string, string>, status?: Partial<StatusPayload>) {
        this.settings = makeSettingsPayload(settings ?? {});
        this.status = makeStatusPayload(status);
    }

    /** Replacement for useGetApi(url). */
    readonly useGetApi = (url: string) => {
        const [response, setResponse] = useState<any>(null);
        useEffect(() => {
            const deliver = () => {
                if (url === "/api/status") {
                    setResponse({ ...this.status });
                } else {
                    // Deep-clone like a fresh fetch(): the settings page keys
                    // state off the settings object reference, so each refresh
                    // must hand back a brand new one.
                    setResponse(JSON.parse(JSON.stringify(this.settings)));
                }
            };
            deliver();
            this.listeners.add(deliver);
            return () => {
                this.listeners.delete(deliver);
            };
        }, [url]);
        return response;
    };

    /** Swap a fresh settings payload in (simulates the backend state). */
    setSettings(settings: Record<string, string>, reboot_required = false) {
        this.settings = makeSettingsPayload(settings, reboot_required);
    }

    notify() {
        for (const l of this.listeners) l();
    }

    /** Reset mutable state + mock call history between tests. */
    reset(settings?: Record<string, string>, status?: Partial<StatusPayload>) {
        this.settings = makeSettingsPayload(settings ?? {});
        this.status = makeStatusPayload(status);
        this.listeners.clear();
        this.apiPost.mockClear();
        this.refreshApi.mockClear();
    }
}

/**
 * The settings page marks up its form controls with `name` attributes but the
 * labels are not associated (no `for`, not wrapping), so getByLabelText is not
 * usable. Look the controls up by name/tag instead.
 */
export function formControl<T extends HTMLElement = HTMLInputElement>(
    name: string,
    tag = "input",
): T {
    const el = document.querySelector(`${tag}[name="${name}"]`);
    if (!el) throw new Error(`No <${tag} name="${name}"> found in the document`);
    return el as T;
}