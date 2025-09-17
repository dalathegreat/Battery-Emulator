import { useGetApi } from './utils/api.tsx'

function get_time(s: number) {
    return (new Date(Date.now() - s)).toLocaleString();
}

export function Events() {
    const data = useGetApi('/api/events', 5000);
    
    return ( <>
        <h2>Events</h2>
        <div class="panel">
            <table>
                <thead>
                    <tr>
                        <th>Type</th>
                        <th>Level</th>
                        <th>Time</th>
                        <th>Count</th>
                        <th>Data</th>
                        <th>Message</th>
                    </tr>
                </thead>
                <tbody>
                    { data && data.events.map( (ev: any, idx: number) => (
                        <tr key={idx} data-level={ev.level.toLowerCase()}>
                            <td>{ ev.type }</td>
                            <td>{ ev.level }</td>
                            <td>{ get_time(ev.age) }</td>
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
