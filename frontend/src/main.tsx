import { render } from 'preact'
import { App } from './app.tsx'
import './css/styles.css'

declare global {
    // Note the capital "W"
    interface Window { latest_event_time: any; }
}

render(<App />, document.getElementById('app')!)
