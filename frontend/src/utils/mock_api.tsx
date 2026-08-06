import { useEffect, useState } from 'preact/hooks'

const SETTINGS = {
    BATTTYPE: '0',
    INVTYPE: '0',
    LOCALIP: '192.168.1.100'
};

const MOCK_DATA: Record<string, any> = {
    '/api/status': ()=>({
        firmware: 'v1.2.3-demo',
        hardware: 'ESP32-S3',
        temp: 42.5 + (Math.random() - 0.5),
        uptime: Date.now() - 1775979583721,
        ssid: 'Demo-WiFi',
        rssi: -55 + Math.floor(Math.random() * 10),
        hostname: 'battery-emulator',
        ip: '192.168.1.100',
        status: 'ok',
        battery: [
            { 
                p: 0,
                protocol: "BMW i3",
                real_soc: 50,
                remaining_capacity: 15000,
                reported_remaining_capacity: 15000,
                reported_soc: 50,
                reported_total_capacity: 90000,
                soc_scaling: false,
                temp_max: 6,
                temp_min: 5,
                total_capacity: 30000,
                v: 370,
                i: 0,
                cell_mv_max: 3720,
                cell_mv_min: 3680
            },
            { 
                p: 0,
                protocol: "BMW i3",
                real_soc: 50,
                remaining_capacity: 15000,
                reported_remaining_capacity: 15000,
                reported_soc: 50,
                reported_total_capacity: 90000,
                soc_scaling: false,
                temp_max: 6,
                temp_min: 5,
                total_capacity: 30000,
                v: 370,
                i: 0,
                cell_mv_max: 3720,
                cell_mv_min: 3680
            },
        ],
        inverter: {
            name: "SolaX Triple Power LFP over CAN bus",
        },
        events: [
            {
                "age": 814,
                "level": "WARNING"
            },
            {
                "age": 1309275,
                "level": "INFO"
            },
            {
                "age": 401653,
                "level": "WARNING"
            },
            {
                "age": 405309,
                "level": "INFO"
            },
            {
                "age": 403727,
                "level": "ERROR"
            }
        ],
        inverter_status: 'inactive',
    }),
    '/api/events': ()=>({
        events: [
            {
                "type": "CAN_BATTERY_MISSING",
                "level": "ERROR",
                "age": 516,
                "count": 225,
                "data": 0,
                "message": "Battery not sending messages via CAN for the last 60 seconds. Check wiring!"
            },
            {
                "type": "CAN_INVERTER_MISSING",
                "level": "WARNING",
                "age": 516,
                "count": 225,
                "data": 2,
                "message": "Inverter not sending messages via CAN for the last 60 seconds. Check wiring!"
            },
            {
                "type": "TASK_OVERRUN",
                "level": "INFO",
                "age": 61944064,
                "count": 1,
                "data": 18,
                "message": "Task took too long to complete. CPU load might be too high. Info message, no action required."
            },
            {
                "type": "BATTERY_EMPTY",
                "level": "INFO",
                "age": 63516848,
                "count": 1,
                "data": 0,
                "message": "Battery is completely discharged"
            },
            {
                "type": "RESET_SW",
                "level": "INFO",
                "age": 63517317,
                "count": 1,
                "data": 3,
                "message": "The board was reset via software, webserver or OTA. Normal operation"
            }
        ]
    }),
    '/api/internal/settings': ()=>({
        batteries: [
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
            "ENNOID BMS via VESC, DIY battery"
        ],
        inverters: [
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
            "Pylon low voltage via RS485"
        ],
        settings: SETTINGS
    }),
    '/api/cells': ()=>({
        battery: [
            {
                "temp_min": 5,
                "temp_max": 6,
                "voltages": Array.from({ length: 96 }, () => Math.floor(Math.random() * (3720 - 3680 + 1)) + 3680)
            }
        ]
    }),
    '/api/log': ()=>(`       0.221 init_Wifi enabled=1, ap=1, ssid=, password=
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
       1.003 Event: Battery is completely discharged
      61.006 Event: Battery not sending messages via CAN for the last 60 seconds. Check wiring!
      61.007 Event: Inverter not sending messages via CAN for the last 60 seconds. Check wiring!
    1573.788 Event: Task took too long to complete. CPU load might be too high. Info message, no action required.
   63560.336 Event: Battery not sending messages via CAN for the last 60 seconds. Check wiring!
   63560.337 Event: Inverter not sending messages via CAN for the last 60 seconds. Check wiring!
\x00`),
};

export function useMockGetApi(url: string, period: number = 0) {
    const [response, setResponse] = useState<any>(null);

    useEffect(() => {
        let timeout: any;
        const update = () => {
            const baseData = MOCK_DATA[url] ? MOCK_DATA[url]() : { error: 'Mock not found' };
            // Add some noise to make it look alive
            const data = JSON.parse(JSON.stringify(baseData));
            if (url === '/api/status') {
                data.uptime += period || 1000;
                data.battery.forEach((b: any) => {
                    b.reported_soc = Math.max(0, Math.min(100, b.reported_soc + b.i * 0.1));
                });
            }
            if (typeof data === 'object') {
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
    console.log(`Mocking POST to ${url}`, data);

    if(url === '/api/internal/settings') {
        Object.assign(SETTINGS, data);
        return MOCK_DATA['/api/internal/settings'];
    }

    throw new Error(`No mock implementation for POST ${url}`);
}
