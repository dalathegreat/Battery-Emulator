import { useDeferred } from '../utils/hooks.tsx';

export function EventCount({status}: {status: any}) {
  // Delay the update by one tick (so when we're on the events page, the
  // latest-event-seen updates before we re-render this badge, so it doesn't
  // flicker.)
  const status_ = useDeferred(status);

  const latest = window.latest_event_time || 0;
  const events = (status_?.events || []).filter((ev: any) => (status_?.uptime - ev.age) > (latest + 10));
  
  const levels: {[key: string]: number} = {'ERROR': 2, 'WARNING': 1};
  const level = Math.max(...events.map((ev: any) => (levels[ev.level] || 0)), 0);
  const worst = level === 2 ? 'error' : level === 1 ? 'warn' : 'info';
  const worst_events = events.filter((ev: any) => (levels[ev.level] || 0) === level);

  if(events.length===0) {
    return null;
  }
  return <span class="badge s" data-status={worst}>{ worst_events.length }</span>;
}
