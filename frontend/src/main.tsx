import { render } from 'preact'
import { App } from './app.tsx'
import { installMockApi, demoDumpCan } from './utils/mock_api.tsx'
import './css/styles.css'

declare global {
    // Note the capital "W"
    interface Window { latest_event_time: any; }
}

if (import.meta.env.VITE_DEMO_MODE === 'true') {
    // Demo mode: run entirely against the in-browser mock, no backend needed.
    installMockApi();

    // The CAN log page is normally served by the device; render a mock log
    // when opened directly in a new tab.
    if (window.location.pathname === '/dump_can') {
        document.title = 'CAN log (demo)';
        const pre = document.createElement('pre');
        pre.textContent = demoDumpCan();
        document.body.appendChild(pre);
    } else {
        render(<App />, document.getElementById('app')!)
    }
} else {
    render(<App />, document.getElementById('app')!)
}
