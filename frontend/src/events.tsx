import { useGetApi } from './utils/api.tsx'

function get_time(now:any, s: number) {
    return (new Date(now - s)).toLocaleString();
}

export function Events() {
    const data = useGetApi('/api/events', 5000);

    if(data?.events) {
        // Update global latest event time
        window.latest_event_time = Math.max(...data?.events.map((ev: any) => (data._now - ev.age)), 0);
    }
    
    const clearEvents = () => {
        if(confirm('Clear all events?')) {
            fetch(import.meta.env.VITE_API_BASE + '/api/events/clear', { method: 'POST' });
        }
    };

    return ( <>
        <div class="heading">
            <h2>Events</h2>
            <button onClick={clearEvents}>Clear events</button>
        </div>
        <div class="panel">
            <table>
                <thead>
                    <tr>
                        <th>Type</th>
                        <th style="width: 7rem">Level</th>
                        <th>Time</th>
                        <th style="width: 4rem">Count</th>
                        <th style="width: 4rem">Data</th>
                        <th>Message</th>
                    </tr>
                </thead>
                <tbody>
                    { data && data.events.map( (ev: any, idx: number) => (
                        <tr key={idx} data-level={ev.level.toLowerCase()}>
                            <td>{ ev.type }</td>
                            <td>{ ev.level }</td>
                            <td>{ get_time(data._now, ev.age) }</td>
                            <td>{ ev.count }</td>
                            <td>{ ev.data }</td>
                            <td>{ ev.message }</td>
                        </tr>
                    )) }
                </tbody>
            </table>            
        </div>
    </> );
}
