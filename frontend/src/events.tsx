import { useEffect, useRef } from 'preact/hooks';

import { refreshApi } from './utils/api.tsx'

function get_time(now: any, s: number) {
    return (new Date(now - s)).toLocaleString();
}

function EventRow({ ev, now }: { ev: any, now: number }) {
    const domRef = useRef<HTMLTableRowElement>(null);

    // Show a brief highlight when a new event is received
    useEffect(() => {
        if (ev.age < 5000) {
            const el = domRef.current;
            if (!el) return;
            el.setAttribute('data-highlight', 'true');
            setTimeout(() => {
                el.removeAttribute('data-highlight');
            }, 50);
        }
    }, [ev.age, now]);

    return (
        <tr data-level={ev.level.toLowerCase()} class='event-row' ref={domRef}>
            <td>{ ev.type }</td>
            <td>{ ev.level }</td>
            <td>{ get_time(now, ev.age) }</td>
            <td>{ ev.count }</td>
            <td>{ ev.data }</td>
            <td>{ ev.message }</td>
        </tr>
    );
}

export function Events({ status }: { status: any }) {
    // Refresh once on mount so we don't wait up to 3s for the next status poll.
    useEffect(() => {
        refreshApi();
    }, []);

    // Sort the events, newest first
    const events = [...(status?.events || [])].sort((a: any, b: any) => (a.age - b.age)); // newest first

    // Log the latest event time to clear the notifications
    useEffect(() => {
        if (events.length) {
            window.latest_event_time = status.uptime - events[0].age;
        }
    }, [events, status?.uptime]);

    const clearEvents = () => {
        if(confirm('Clear all events?')) {
            fetch(import.meta.env.VITE_API_BASE + '/api/events/clear', { method: 'POST' })
                .then(refreshApi);
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
                    { events.map( (ev: any) => (
                        <EventRow key={ `${ev.type}-${ev.count}` } ev={ ev } now={ status._now } />
                    )) }
                </tbody>
            </table>
        </div>
    </> );
}
