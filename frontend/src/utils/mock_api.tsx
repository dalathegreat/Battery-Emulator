import { useEffect, useState } from 'preact/hooks'

const MOCK_DATA: Record<string, any> = {
    '/api/status': {
        firmware: 'v1.2.3-demo',
        hardware: 'ESP32-S3',
        temp: 42.5,
        uptime: 1234567,
        ssid: 'Demo-WiFi',
        rssi: -55,
        hostname: 'battery-emulator',
        ip: '192.168.1.100',
        status: 'ok',
        battery: [
            { reported_soc: 75, i: 0.1 },
            { reported_soc: 42, i: -1.2 }
        ],
        events: []
    },
    '/api/events': {
        events: [
            { type: 'Power', level: 'INFO', age: 1000, count: 1, data: '0', message: 'System started' },
            { type: 'Battery', level: 'WARN', age: 5000, count: 3, data: '1', message: 'Low voltage' },
            { type: 'CAN', level: 'ERROR', age: 15000, count: 1, data: '2', message: 'Timeout' }
        ]
    },
    '/api/internal/settings': {
        batteries: ['Tesla Model S', 'Nissan Leaf', 'BYD B-Box'],
        inverters: ['Victron MultiPlus', 'SMA Sunny Island', 'Pylontech'],
        settings: {
            BATTTYPE: '0',
            INVTYPE: '0',
            LOCALIP: '192.168.1.100'
        }
    },
    '/api/v1/cellmonitor': {
        cells: Array.from({ length: 16 }, (_, i) => ({
            v: 3.2 + Math.random() * 0.2,
            t: 25 + Math.random() * 5,
            bal: i % 4 === 0
        }))
    }
};

export function useMockGetApi(url: string, period: number = 0) {
    const [response, setResponse] = useState<any>(null);

    useEffect(() => {
        let timeout: any;
        const update = () => {
            const baseData = MOCK_DATA[url] || { error: 'Mock not found' };
            // Add some noise to make it look alive
            const data = JSON.parse(JSON.stringify(baseData));
            if (url === '/api/status') {
                data.uptime += period || 1000;
                data.temp += (Math.random() - 0.5);
                data.battery.forEach((b: any) => {
                    b.reported_soc = Math.max(0, Math.min(100, b.reported_soc + b.i * 0.1));
                });
            }
            // if (url === '/api/events') {
            //     data.events.forEach((ev: any) => {
            //         ev.age += period || 5000;
            //     });
            // }
            data._now = Date.now();
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
