import { useEffect, useState } from 'preact/hooks'

// ---------------------------------------------------------------------------
// Mock API layer.
//
// Used when VITE_DEMO_MODE === 'true' so the SPA can be demonstrated without
// any backend (ESP32) present. It provides:
//
//   * useMockGetApi()  - drop-in replacement for useGetApi() backed by an
//                        in-memory, slowly evolving device state.
//   * apiPostMock()    - drop-in replacement for apiPost().
//   * installMockApi() - intercepts raw window.fetch()/XMLHttpRequest calls
//                        (pause/estop/reboot/events-clear/OTA/CAN-sender) so
//                        every interaction with the device is simulated.
//                        Call once from main.tsx in demo mode.
// ---------------------------------------------------------------------------

// import {
//     DATALAYER_INFO_BOLTAMPERA_FIELDS,
//     DATALAYER_INFO_BMWPHEV_FIELDS,
//     DATALAYER_INFO_BMWIX_FIELDS,
//     DATALAYER_INFO_BYDATTO3_FIELDS,
//     DATALAYER_INFO_CELLPOWER_FIELDS,
//     DATALAYER_INFO_CHADEMO_FIELDS,
//     DATALAYER_INFO_CMPSMART_FIELDS,
//     DATALAYER_INFO_ECMP_FIELDS,
//     DATALAYER_INFO_FORD_MACH_E_FIELDS,
//     DATALAYER_INFO_GEELY_GEOMETRY_C_FIELDS,
//     DATALAYER_INFO_GEELY_SEA_FIELDS,
//     DATALAYER_INFO_KIAHYUNDAI64_FIELDS,
//     DATALAYER_INFO_NISSAN_LEAF_FIELDS,
//     DATALAYER_INFO_MEB_FIELDS,
//     DATALAYER_INFO_RIVIAN_FIELDS,
//     DATALAYER_INFO_VOLVO_POLESTAR_FIELDS,
//     DATALAYER_INFO_VOLVO_HYBRID_FIELDS,
//     DATALAYER_INFO_ZOE_FIELDS,
//     DATALAYER_INFO_ZOE_PH2_FIELDS,
// } from '../ext/datalayer.ts';

// Same battery-type -> field-list mapping as used by the Extended info page.
const FIELD_LISTS: Record<number, any[]> = {
    // 4: DATALAYER_INFO_BOLTAMPERA_FIELDS,
    // 43: DATALAYER_INFO_BMWPHEV_FIELDS,
    // 3: DATALAYER_INFO_BMWIX_FIELDS,
    // 5: DATALAYER_INFO_BYDATTO3_FIELDS,
    // 6: DATALAYER_INFO_CELLPOWER_FIELDS,
    // 7: DATALAYER_INFO_CHADEMO_FIELDS,
    // 45: DATALAYER_INFO_CMPSMART_FIELDS,
    // 13: DATALAYER_INFO_ECMP_FIELDS,
    // 44: DATALAYER_INFO_FORD_MACH_E_FIELDS,
    // 10: DATALAYER_INFO_GEELY_GEOMETRY_C_FIELDS,
    // 50: DATALAYER_INFO_GEELY_SEA_FIELDS,
    // 17: DATALAYER_INFO_KIAHYUNDAI64_FIELDS,
    // 21: DATALAYER_INFO_NISSAN_LEAF_FIELDS,
    // 19: DATALAYER_INFO_MEB_FIELDS,
    // 42: DATALAYER_INFO_RIVIAN_FIELDS,
    // 35: DATALAYER_INFO_VOLVO_POLESTAR_FIELDS,
    // 36: DATALAYER_INFO_VOLVO_HYBRID_FIELDS,
    // 28: DATALAYER_INFO_ZOE_FIELDS,
    // 29: DATALAYER_INFO_ZOE_PH2_FIELDS,
};

const LENGTHS: Record<string, number> = {
    'u8': 1, 'u16': 2, 'u32': 4, 'u64': 8,
    'i8': 1, 'i16': 2, 'i32': 4,
    'f': 4, 'b': 1, ' ': 1,
};

// ---------------------------------------------------------------------------
// Mutable mock device state
// ---------------------------------------------------------------------------

let BOOT_TIME = Date.now();
const UPTIME_OFFSET = 2 * 24 * 3600 * 1000; // pretend we've been up for 2 days

let paused = false;
let estop = false;
let rebootAt = 0;               // when a reboot was requested; device "offline" for a while after
let lastEventAt = 0;

interface MockEvent { ts: number; type: string; level: string; count: number; data: number; message: string; }
let events: MockEvent[] = [
    { ts: BOOT_TIME - 2000,                       type: 'RESET_SW',             level: 'INFO',    count: 1, data: 3, message: 'The board was reset via software, webserver or OTA. Normal operation' },
    { ts: BOOT_TIME - 3 * 60 * 1000,              type: 'BATTERY_EMPTY',        level: 'INFO',    count: 1, data: 0, message: 'Battery is completely discharged' },
    { ts: BOOT_TIME - 25 * 60 * 1000,             type: 'TASK_OVERRUN',         level: 'INFO',    count: 3, data: 18, message: 'Task took too long to complete. CPU load might be too high. Info message, no action required.' },
    { ts: BOOT_TIME - 2 * 3600 * 1000,            type: 'CAN_INVERTER_MISSING', level: 'WARNING', count: 2, data: 2, message: 'Inverter not sending messages via CAN for the last 60 seconds. Check wiring!' },
    { ts: BOOT_TIME - 26 * 3600 * 1000,           type: 'CAN_BATTERY_MISSING',  level: 'ERROR',   count: 225, data: 0, message: 'Battery not sending messages via CAN for the last 60 seconds. Check wiring!' },
];

// Simulated battery: a 24 kWh Nissan LEAF pack.
const SIM = {
    total_wh: 24000,
    nominal_v: 370,
    cells: 96,
    soc_wh: 24000 * 0.63,
};
let lastSimAt = Date.now();

const SETTINGS: Record<string, string> = {
    BATTTYPE: '21',           // Nissan LEAF battery
    BATTCOMM: '3',            // Native CAN
    INVTYPE: '18',            // SolaX Triple Power LFP over CAN bus
    INVCOMM: '3',
    BATTERY_WH_MAX: '24000',
    MAXCHARGEAMP: '100',
    MAXDISCHARGEAMP: '100',
    BATTCHEM: '1',
    CHGTYPE: '0',
    SHUNTTYPE: '0',
    SSID: 'DemoHomeWifi',
    WIFIAPENABLED: '0',
    HOSTNAME: 'battery-emulator',
    STATICIP: '0',
    LOCALIP: '192.168.1.100',
    GATEWAY: '192.168.1.1',
    SUBNET: '255.255.255.0',
    DNS: '192.168.1.1',
    WEBAUTH: '0',
    MQTTENABLED: '0',
    WEBENABLED: '1',
    USBENABLED: '1',
    CANLOGUSB: '1',
    LOWPASSFILTER: '1',
    // BYD Atto 3 auto-calibration + isolation settings (see webserver_settings.cpp)
    BYDAUTOCALEN: '1',
    BYDAUTOCALDRIFT: '5',
    BYDKEEPISOOFF: '0',
    TMP_CALTARGETSOC: '100',
    TMP_CALTARGETAH: '150',
};
let rebootRequired = false;

// ---------------------------------------------------------------------------
// Simulation helpers
// ---------------------------------------------------------------------------

function powerAt(now: number): number {
    if (paused || estop) return 0;
    const t = now / 5000;
    // Smooth charge/discharge cycling, plus a little noise.
    const p = Math.sin(t * 0.21) * 1700 + Math.sin(t * 0.053 + 1.3) * 900 + (Math.random() - 0.5) * 150;
    return Math.round(p / 10) * 10;
}

function advanceSim(now: number) {
    const dt = Math.max(0, Math.min((now - lastSimAt) / 1000, 10)); // seconds, capped
    lastSimAt = now;
    const p = powerAt(now);
    SIM.soc_wh = Math.max(0.02 * SIM.total_wh, Math.min(0.98 * SIM.total_wh, SIM.soc_wh + p * dt / 3600));
    return p;
}

function statusPayload(): any {
    const now = Date.now();
    const p = advanceSim(now);
    const v = SIM.nominal_v + Math.sin(now * 0.0001) * 2 + (p / SIM.nominal_v) * 0.05;
    const i = p / v;
    const soc_real = SIM.soc_wh / SIM.total_wh * 100;
    const reported_soc = Math.min(95, Math.max(5, 20 + soc_real * 0.7));
    const temp_min = 7 + Math.random() * 3;
    const temp_max = temp_min + 3 + Math.random() * 3;

    // Keep the event list alive: throw in a benign INFO event once in a while.
    if (now - lastEventAt > 20000) {
        lastEventAt = now;
        events = [...events, {
            ts: now,
            type: 'CAN_OK',
            level: 'INFO',
            count: 1,
            data: 0,
            message: 'Battery and inverter communicating normally',
        }];
    }
    // Events can't have a timestamp in the future.
    events = events.filter(ev => ev.ts <= now);

    return {
        firmware: 'v1.2.3-demo',
        hardware: 'LilyGo T_2CAN',
        temp: 42.5 + (Math.random() - 0.5) * 4,
        uptime: (now - BOOT_TIME) + UPTIME_OFFSET,
        ssid: 'DemoHomeWifi',
        rssi: -55 + Math.floor(Math.random() * 10),
        hostname: 'battery-emulator',
        ip: '192.168.1.100',
        gateway: '192.168.1.1',
        subnet: '255.255.255.0',
        dns: '192.168.1.1',
        status: 'ACTIVE',
        pause: paused,
        estop: estop,
        battery: [{
            p,
            i,
            v,
            protocol: 'Nissan LEAF battery',
            status: 'ok',
            real_soc: soc_real,
            reported_soc,
            remaining_capacity: SIM.soc_wh,
            reported_remaining_capacity: SIM.soc_wh,
            reported_total_capacity: SIM.total_wh,
            total_capacity: SIM.total_wh,
            soc_scaling: true,
            temp_min,
            temp_max,
            cell_mv_max: Math.round(3700 + Math.random() * 25),
            cell_mv_min: Math.round(3650 + Math.random() * 25),
            charge_p_max: 3000,
            discharge_p_max: 3000,
            charge_i_max: 20,
            discharge_i_max: 20,
        }],
        contactor: { state: estop ? 0 : 5 },
        inverter: {
            name: 'SolaX Triple Power LFP over CAN bus',
            status: (paused || estop) ? 'INACTIVE' : 'ACTIVE',
        },
        events: events.map(ev => ({
            type: ev.type,
            level: ev.level,
            age: now - ev.ts,
            count: ev.count,
            data: ev.data,
            message: ev.message,
        })),
    };
}

function settingsPayload(): any {
    return {
        batteries: [
            'None',
            null,
            'BMW i3',
            'BMW iX and i4-7 platform',
            'Chevrolet Bolt EV/Opel Ampera-e',
            'BYD Atto 3/Seal/Dolphin',
            'Cellpower BMS',
            'Chademo V2X mode',
            'CMFA platform, 27 kWh battery',
            'FoxESS HV2600/ECS4100 OEM battery',
            'Geely Geometry C',
            'DIY battery with Orion BMS (Victron setting)',
            'Sono Motors Sion 64kWh LFP ',
            'Stellantis ECMP battery',
            'I-Miev / C-Zero / Ion Triplet',
            'Jaguar I-PACE',
            'Kia/Hyundai EGMP platform',
            'Kia/Hyundai 64/40kWh battery',
            'Kia/Hyundai Hybrid',
            'Volkswagen Group MEB platform via CAN-FD',
            'MG 5 battery',
            'Nissan LEAF battery',
            'Pylon compatible battery',
            'DALY RS485',
            'RJXZS BMS, DIY battery',
            'Range Rover 13kWh PHEV battery (L494/L405)',
            'Renault Kangoo',
            'Renault Twizy',
            'Renault Zoe Gen1 22/40kWh',
            'Renault Zoe Gen2 50kWh',
            'Santa Fe PHEV',
            'SIMPBMS battery',
            'Tesla Model 3/Y',
            'Tesla Model S/X',
            'Fake battery for testing purposes',
            'Volvo / Polestar 69/78kWh SPA battery',
            'Volvo PHEV battery',
            'MG HS PHEV 16.6kWh battery',
            'Samsung SDI LV Battery',
            'Hyundai Ioniq Electric 28kWh',
            'Kia 64kWh FD battery',
            'Relion LV protocol via 250kbps CAN',
            'Rivian R1T large 135kWh battery',
            'BMW PHEV Battery',
            'Ford Mustang Mach-E battery',
            'Stellantis CMP Smart Car Battery',
            null,
            'Think City',
            'Tesla Model S/X 2012-2020',
            'Growatt HV ARK battery (battery-facing CAN)',
            'Volvo/Zeekr/Geely SEA battery',
            'Thunderstruck BMS',
            'ENNOID BMS via VESC, DIY battery',
        ],
        inverters: [
            'None',
            'Afore battery over CAN',
            'BYD Battery-Box Premium HVS over CAN Bus',
            'BYD 11kWh HVM battery over Modbus RTU',
            'Ferroamp Pylon battery over CAN bus',
            'FoxESS compatible HV2600/ECS4100 battery',
            'Growatt High Voltage protocol via CAN',
            'Growatt Low Voltage (48V) protocol via CAN',
            'Growatt WIT compatible battery via CAN',
            'BYD battery via Kostal RS485',
            'Pylontech HV battery over CAN bus',
            'Pylontech LV battery over CAN bus',
            'Schneider V2 SE BMS CAN',
            null,
            'SMA compatible BYD Battery-Box H',
            'SMA Low Voltage (48V) protocol via CAN',
            'SMA compatible BYD Battery-Box HVS',
            'Sofar BMS (Extended) via CAN, Battery ID',
            'SolaX Triple Power LFP over CAN bus',
            'Solxpow compatible battery',
            'Sol-Ark LV protocol over CAN bus',
            'Sungrow SBRXXX emulation over CAN bus',
            'VCU mode: Nissan LEAF battery',
            'Pylon low voltage via RS485',
        ],
        settings: SETTINGS,
        reboot_required: rebootRequired,
    };
}

// ---------------------------------------------------------------------------
// Extended info (/api/batext): a binary blob describing the datalayer of the
// currently selected battery. Mirrors what the ESP32 would serve.
// ---------------------------------------------------------------------------

function effectiveBtype(): number {
    const t = parseInt(SETTINGS.BATTTYPE);
    return FIELD_LISTS[t] ? t : 21;
}

function genValue(name: string, type: string, arrLen: number): number | bigint {
    const rnd = Math.random;
    if (/serial|part|idcode|bmsid/i.test(name) && arrLen > 1) {
        // ASCII string-ish: digits (and sometimes letters)
        const alphabet = '0123456789ABCDEFGHJKLMNPRSTUVWXYZ';
        return alphabet.charCodeAt(Math.floor(rnd() * alphabet.length));
    }
    if (/dtc/i.test(name)) return rnd() < 0.9 ? 0 : 0x80000000 + Math.floor(rnd() * 0x7fffffff);
    if (/challenge/i.test(name)) return Math.floor(rnd() * 0xffffffff);
    if (/temp/i.test(name)) return Math.round(rnd() * 20 + 5);
    if (/volt/i.test(name)) return type === 'u16' ? Math.round(3000 + rnd() * 800) : Math.round(350 + rnd() * 60);
    if (/current/i.test(name)) return Math.round((rnd() - 0.5) * 100);
    if (/soc|gids/i.test(name)) return Math.round(20 + rnd() * 70);
    if (/capacity/i.test(name)) return Math.round(20 + rnd() * 25);
    if (/resistance/i.test(name)) return Math.round(20 + rnd() * 80);
    if (/kohm|isolation|iso|insul/i.test(name)) return Math.round(500 + rnd() * 1500);
    if (/power|limit/i.test(name)) return Math.round(rnd() * 1000);
    if (/millis|age|time/i.test(name)) return Math.round(rnd() * 1000);
    if (/state|status|flag|lock|request|relay|heat|interlock|full|empty|gen|mode|failsafe|crash|hvil/i.test(name)) return rnd() < 0.85 ? 0 : 1;
    switch (type) {
        case 'u64': return BigInt(Math.floor(rnd() * 1e6));
        case 'u32': case 'i32': return Math.floor(rnd() * 100000);
        case 'u16': case 'i16': return Math.floor(rnd() * 4000);
        default: return Math.floor(rnd() * 250);
    }
}

function setValue(dv: DataView, type: string, offset: number, v: number | bigint) {
    switch (type) {
        case 'u8': case 'b': dv.setUint8(offset, Number(v)); break;
        case 'u16': dv.setUint16(offset, Number(v), true); break;
        case 'u32': dv.setUint32(offset, Number(v), true); break;
        case 'u64': dv.setBigUint64(offset, BigInt(v), true); break;
        case 'i8': dv.setInt8(offset, Number(v)); break;
        case 'i16': dv.setInt16(offset, Number(v), true); break;
        case 'i32': dv.setInt32(offset, Number(v), true); break;
        case 'f': dv.setFloat32(offset, Number(v), true); break;
        // ' ' = padding, leave as zero
    }
}

function buildBatext(btype: number): ArrayBuffer {
    const fields = FIELD_LISTS[btype] || [];
    let total = 4; // u32 header: battery type
    fields.forEach(([, type, arr]: any[]) => { total += LENGTHS[type] * (arr || 1); });

    const buf = new ArrayBuffer(total);
    const dv = new DataView(buf);
    dv.setUint32(0, btype, true);

    let offset = 4;
    fields.forEach(([name, type, arr]: any[]) => {
        const len = LENGTHS[type];
        const n = arr || 1;
        for (let i = 0; i < n; i++) {
            setValue(dv, type, offset, genValue(name, type, n));
            offset += len;
        }
    });

    // Return the raw bytes, matching what the ESP32 serves over HTTP
    // (application/octet-stream); consumers read it via DataView/ArrayBuffer.
    return buf;
}

// ---------------------------------------------------------------------------
// GET endpoints
// ---------------------------------------------------------------------------

const MOCK_DATA: Record<string, any> = {
    '/api/status': () => statusPayload(),

    '/api/events': () => ({
        events: statusPayload().events,
    }),

    '/api/internal/settings': () => settingsPayload(),

    '/api/cells': () => ({
        battery: [{
            temp_min: 7 + Math.random() * 3,
            temp_max: 12 + Math.random() * 4,
            voltages: Array.from({ length: SIM.cells }, () => Math.floor(Math.random() * (3720 - 3650 + 1)) + 3650),
        }],
    }),

    '/api/log': () => (`       0.221 init_Wifi enabled=1, ap=1, ssid=, password=
       0.223 transmitter registered, total: 1
       0.224 CAN receiver registered, total: 1
       0.224 Requesting 500 kbps for inverter CAN interface ()
       0.224 transmitter registered, total: 2
       0.225 CAN receiver registered, total: 2
       0.228 Native Can ok
       0.229 Bit Rate prescaler: 4
       0.229 Time Segment 1:     13
       0.229 Time Segment 2:     6
       0.229 RJW:                4
       0.229 Triple Sampling:    no
       0.229 Actual bit rate:    500000 bit/s
       0.229 Exact bit rate ?    yes
       0.229 Sample point:       70%
       0.230 Dual CAN Bus (ESP32+MCP2515) selected
       0.297 init_Wifi set event handlers
       0.297 start Wifi
       0.297 init_Wifi complete
       0.534 Can ok
       0.535 Event: The board was reset via software, webserver or OTA. Normal operation
       0.539 Setup complete!
       0.540 Battery: Nissan LEAF battery detected on interface 0
       0.541 Inverter: SolaX Triple Power LFP over CAN bus detected on interface 0
       0.541 Contactors closed
       1.003 Event: Battery is completely discharged
      61.006 Event: Battery not sending messages via CAN for the last 60 seconds. Check wiring!
      61.007 Event: Inverter not sending messages via CAN for the last 60 seconds. Check wiring!
    1573.788 Event: Task took too long to complete. CPU load might be too high. Info message, no action required.
   63560.336 Event: Battery not sending messages via CAN for the last 60 seconds. Check wiring!
   63560.337 Event: Inverter not sending messages via CAN for the last 60 seconds. Check wiring!
\x00`),

    '/api/batext': () => buildBatext(effectiveBtype()),

    '/api/batold': () => `<div class="panel">
<h3>Legacy battery view</h3>
<p>This is the "old" extended info view, still served by the backend.</p>
<table class="grid">
<tr><th>GIDS</th><th>HX</th><th>Charging</th><th>Range</th></tr>
<tr><td id="legacy-gids">--</td><td id="legacy-hx">--</td><td id="legacy-chg">--</td><td id="legacy-range">--</td></tr>
</table>
<script>
try {
    document.getElementById('legacy-gids').textContent = Math.round(20 + Math.random() * 60);
    document.getElementById('legacy-hx').textContent = (80 + Math.random() * 15).toFixed(1);
    document.getElementById('legacy-chg').textContent = Math.random() > 0.5 ? 'yes' : 'no';
    document.getElementById('legacy-range').textContent = Math.round(40 + Math.random() * 80) + ' km';
} catch (e) {}
</script>
</div>`,

    '/api/batteries/': () => ({
        battery: [{
            id: 1,
            commands: {
                reset_soc: true,
                reset_soh: true,
                reset_crash: true,
                clear_isolation: true,
                calibrate_soc: true,
                contactor_close: true,
                contactor_open: true,
                balancing: true,
                balancing_active: !paused,
            },
        }],
    }),

    // OTA release listing (used by the OTA page in demo mode).
    'https://api.github.com/repos/dalathegreat/BE-Web-Installer/git/trees/main?recursive=1': () => ({
        tree: [
            { path: 'releases/0.9.0/LilygoT-2CAN.ota.bin' },
            { path: 'releases/0.9.1/LilygoT-2CAN.ota.bin' },
            { path: 'releases/1.0.0/LilygoT-2CAN.ota.bin' },
            { path: 'releases/1.0.1/LilygoT-2CAN.ota.bin' },
            { path: 'releases/1.1.0/LilygoT-2CAN.ota.bin' },
            { path: 'releases/1.2.0/LilygoT-2CAN.ota.bin' },
            { path: 'releases/1.2.1/LilygoT-2CAN.ota.bin' },
            { path: 'releases/1.2.2/LilygoT-2CAN.ota.bin' },
            { path: 'releases/1.2.3/LilygoT-2CAN.ota.bin' },
            { path: 'releases/1.2.4/LilygoT-2CAN.ota.bin' },
            { path: 'releases/1.3.0/LilygoT-2CAN.ota.bin' },
            { path: 'releases/2.0.0/LilygoT-2CAN.ota.bin' },
        ],
    }),
    'https://api.github.com/repos/dalathegreat/Battery-Emulator/releases': () => [],
};

// ---------------------------------------------------------------------------
// POST endpoints + raw fetch handling
// ---------------------------------------------------------------------------

const JSON_HEADERS = { 'Content-Type': 'application/json' };
const json = (data: any, status = 200) => new Response(JSON.stringify(data), { status, headers: JSON_HEADERS });

function mockPost(url: string, init?: any): Response {
    const body = (init?.body !== undefined && init?.body !== null) ? ('' + init.body) : '';
    if (url === '/api/pause') {
        paused = body.trim() === '1';
        return json({ ok: true });
    }
    if (url === '/api/estop') {
        estop = body.trim() === '1';
        return json({ ok: true });
    }
    if (url === '/api/reboot') {
        const uptime = Date.now() - BOOT_TIME + UPTIME_OFFSET;
        rebootAt = Date.now();
        BOOT_TIME = Date.now();
        return json({ uptime });
    }
    if (url === '/api/events/clear') {
        events = [];
        return json({ ok: true });
    }
    if (/^\/api\/batteries\/\d+\/[a-z_]+$/.test(url)) {
        return new Response(null, { status: 204 });
    }
    if (url.startsWith('/ota/start')) {
        return json({ ok: true });
    }
    return json({ error: 'No mock implementation for POST ' + url }, 404);
}

function mockGet(url: string): Response {
    if (url === '/dump_can') {
        return new Response(demoDumpCan(), { headers: { 'Content-Type': 'text/plain' } });
    }
    if (url === '/api/status') {
        // Pretend the device is unreachable for a moment after a reboot.
        if (Date.now() - rebootAt < 2500) {
            return new Response('', { status: 503 });
        }
        return json(statusPayload());
    }
    const d = MOCK_DATA[url];
    if (d !== undefined) {
        const v = typeof d === 'function' ? d() : d;
        return v instanceof ArrayBuffer
            ? new Response(v, { headers: { 'Content-Type': 'application/octet-stream' } })
            : json(v);
    }
    return json({ error: 'Mock not found for ' + url }, 404);
}

// Normalise a URL: strip the configured API base prefix so both relative
// ("/api/status") and base-prefixed ("http://192.168.0.107/api/status")
// requests are recognised.
function normalizeUrl(input: any): string {
    const raw = typeof input === 'string' ? input : (input?.url ?? '');
    const base = import.meta.env.VITE_API_BASE;
    if (base && raw.startsWith(base)) return raw.slice(base.length);
    return raw;
}

function shouldMock(u: string): boolean {
    return u.startsWith('/api/') || u.startsWith('/ota/') || u === '/dump_can' || u.startsWith('/dump_can?');
}

// ---------------------------------------------------------------------------
// Demo CAN log dump (for the CAN sender and /dump_can endpoints)
// ---------------------------------------------------------------------------

export function demoDumpCan(): string {
    const ids = [0x0116, 0x01F2, 0x01F3, 0x02BC, 0x02C1, 0x0314, 0x0328, 0x03A4, 0x03B2, 0x0446,
        0x04BC, 0x0528, 0x054C, 0x0647, 0x0654, 0x0668, 0x06A1, 0x0788, 0x07F2, 0x0829,
        0x0924, 0x0A2C, 0x0B2C, 0x0C02, 0x0D02, 0x0E04, 0x0F04, 0x1004, 0x1104, 0x1206];
    const lines = ['(0.000) RX0 0000 [8] 00 00 00 00 00 00 00 00'];
    const end = 4.5 + Math.random() * 3;
    let t = 0;
    while (t < end) {
        const id = ids[Math.floor(Math.random() * ids.length)];
        const bus = Math.random() < 0.5 ? 0 : 2;
        const data = Array.from({ length: 8 }, () =>
            Math.floor(Math.random() * 256).toString(16).padStart(2, '0').toUpperCase());
        lines.push(`(${t.toFixed(3)}) RX${bus} ${id.toString(16).padStart(4, '0').toUpperCase()} [8] ${data.join(' ')}`);
        t += 0.005 + Math.random() * 0.015;
    }
    return lines.join('\n') + '\n';
}

// ---------------------------------------------------------------------------
// Interceptors: make raw fetch()/XMLHttpRequest calls hit the mock layer.
// ---------------------------------------------------------------------------

export function installMockApi() {
    if (typeof window === 'undefined') return;
    if ((window as any).__mockApiInstalled) return;
    (window as any).__mockApiInstalled = true;

    // --- fetch() ---
    const realFetch = window.fetch.bind(window);
    window.fetch = ((input: any, init?: any) => {
        const u = normalizeUrl(input);
        if (shouldMock(u)) {
            const method = ((init?.method) || (typeof input === 'string' ? undefined : input?.method) || 'GET').toUpperCase();
            return Promise.resolve(method === 'POST' ? mockPost(u, init) : mockGet(u));
        }
        return realFetch(input, init);
    }) as typeof fetch;

    // --- XMLHttpRequest (CAN sender uploads, OTA uploads) ---
    const realOpen = XMLHttpRequest.prototype.open;
    const realSend = XMLHttpRequest.prototype.send;
    const realAbort = XMLHttpRequest.prototype.abort;

    XMLHttpRequest.prototype.open = function (this: any, method: string, url: string, ...rest: any[]) {
        this.__mockUrl = normalizeUrl(url);
        return realOpen.apply(this, [method, url, ...rest] as any);
    };

    XMLHttpRequest.prototype.send = function (this: any, body: any) {
        const url = this.__mockUrl;
        if (shouldMock(url) && (url.startsWith('/api/cansend') || url.startsWith('/ota/upload'))) {
            const xhr = this;
            const total = body?.size || body?.byteLength || 1024 * 1024;
            let progress = 0;
            const iv = setInterval(() => {
                progress = Math.min(1, progress + 0.04 + Math.random() * 0.1);
                const loaded = Math.floor(progress * total);
                if (xhr.upload?.onprogress) {
                    xhr.upload.onprogress(new ProgressEvent('progress', { loaded, total, lengthComputable: true }));
                }
                if (progress >= 1) {
                    clearInterval(iv);
                    try {
                        xhr.status = 200;
                        xhr.response = 'OK';
                        xhr.responseText = 'OK';
                        xhr.readyState = 4;
                    } catch (e) { /* not all props are writable in every browser */ }
                    xhr.onload?.(new ProgressEvent('load'));
                    xhr.__mockAbort = null;
                }
            }, 120);
            xhr.__mockAbort = () => {
                clearInterval(iv);
                xhr.onabort?.(new ProgressEvent('abort'));
            };
            return;
        }
        return realSend.apply(this, [body] as any);
    };

    XMLHttpRequest.prototype.abort = function (this: any) {
        if (this.__mockAbort) {
            this.__mockAbort();
            return;
        }
        return realAbort.apply(this, []);
    };
}

// ---------------------------------------------------------------------------
// React hooks + apiPost drop-ins (used by api.tsx in demo mode)
// ---------------------------------------------------------------------------

export function useMockGetApi(url: string, period: number = 0) {
    const [response, setResponse] = useState<any>(null);

    useEffect(() => {
        let timeout: any;
        const update = () => {
            const baseData = MOCK_DATA[url] ? MOCK_DATA[url]() : { error: 'Mock not found' };
            // Deep clone so callers can't mutate shared state. Binary endpoints
            // return an ArrayBuffer, which JSON round-tripping would destroy, so
            // copy those byte-for-byte instead.
            const data = baseData instanceof ArrayBuffer
                ? baseData.slice(0)
                : JSON.parse(JSON.stringify(baseData));
            if (typeof data === 'object' && !(data instanceof ArrayBuffer)) {
                data._now = Date.now();
            }
            setResponse(data);
            if (period > 0) {
                timeout = setTimeout(update, period);
            }
        };

        const h = () => update();
        window.addEventListener('api-invalidate', h);
        update();
        return () => {
            window.removeEventListener('api-invalidate', h);
            clearTimeout(timeout);
        };
    }, [url, period]);

    return response;
}

export async function apiPostMock(url: string, data: any) {
    const u = normalizeUrl(url);
    if (u === '/api/internal/settings') {
        Object.assign(SETTINGS, data || {});
        rebootRequired = true;
        return settingsPayload();
    }
    console.log(`Mocking POST to ${url}`, data);
    const resp = mockPost(u, { method: 'POST', body: data ? JSON.stringify(data) : undefined });
    return resp.json();
}
