import { CanLog } from './can_log.tsx'
import { CanSender } from './can_sender.tsx'
import { CellMonitor } from './cellmonitor.tsx'
import { Dashboard } from './dashboard.tsx'
import { Events } from './events.tsx'
import { Extended } from './extended.tsx'
import { Log } from './log.tsx'
import { Ota } from './ota.tsx'
import { Settings } from './settings.tsx'

import { useGetApi } from './utils/api.tsx'
import { Link, useLocation } from './utils/location.tsx';

function Tray({data}: {data: any}) {
  return (
    <div id="tray">
      { (data?.battery || []).map((battery: any, idx: number) => (
        <div class="battery-level" style={ {"--perc": battery.soc + '%'} } key={idx}>{ battery.soc.toFixed(0) }%{
          battery.i > 0.05 ? <span>▲</span> : battery.i < -0.05 ? <span>▼</span> : ''
        }</div>
      )) }
    </div>
  );
}

function EventCount({data}: {data: any}) {
  const latest = window.latest_event_time || 0;
  const events = (data?.events || []).filter((ev: any) => ((data._now - ev.age) > (latest + 10)));
  const levels: {[key: string]: number} = {'ERROR': 2, 'WARNING': 1};
  const level = Math.max(...events.map((ev: any) => (levels[ev.level] || 0)), 0);
  const worst = level === 2 ? 'error' : level === 1 ? 'warn' : 'info';

  if(events.length===0) {
    return null;
  }
  return <span class="badge" data-status={worst}>{ events.length }</span>;
}

export function App() {
  const data = useGetApi("/api/live", 3000);
  const location = useLocation();
  return (
    <>
      <div class="topbar">
        <h1>🔋 Battery Emulator</h1>
        <Tray data={data} />
        <div class="status">SYSTEM OK</div>
      </div>

      <div class="columns">
        <div class="menu">
          <div>
            <Link href="/" data-cur>Dashboard</Link>
            <Link href="/events">Events <EventCount data={data} /></Link>
            <Link href="/cellmonitor">Cell monitor</Link>
            <Link href="/extended">Extended info</Link>
            <Link href="/settings">Settings</Link>
            <Link href="/canlog">CAN log</Link>
            <Link href="/cansender">CAN sender</Link>
            <Link href="/log">System Log</Link>
            <Link href="/ota">OTA upgrade</Link>
            <a href="#" class="button" style="margin: auto 0 0.75rem; background-color: #bf7c13; color: #ffffff;">Pause</a>
            <a href="#" class="button" style="background-color: #b50909; color: #ffffff;">Open contactors</a>
          </div>
        </div>
        <div class="content">
          { location==="/canlog" && <CanLog /> }
          { location==="/cansender" && <CanSender /> }
          { location==="/cellmonitor" && <CellMonitor /> }
          { location==="/events" && <Events /> }
          { location==="/extended" && <Extended /> }
          { location==="/log" && <Log /> }
          { location==="/ota" && <Ota /> }
          { location==="/settings" && <Settings  /> }
          { location==="/" && <Dashboard /> }
        </div>
      </div>

    </>
  );
}

/*
      <div class="menu">
      <Link class="button" href="/">Dashboard</Link>
      <Link class="button" href="/ota">OTA Update</Link>
      <Link class="button" href="/cellmonitor">Cell Monitor</Link>
      <Link class="button" href="/settings">Settings</Link>
      </div>

*/
