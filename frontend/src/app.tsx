import { CanLog } from './can_log.tsx'
import { CanSender } from './can_sender.tsx'
import { CellMonitor } from './cellmonitor.tsx'
import { Dashboard } from './dashboard.tsx'
import { Events } from './events.tsx'
import { Extended } from './extended.tsx'
import { Log } from './log.tsx'
import { Ota } from './ota.tsx'
import { Settings } from './settings.tsx'

import { Button } from './components/button.tsx'

import { useGetApi } from './utils/api.tsx'
import { Link, useLocation } from './utils/location.tsx';
import { reboot } from './utils/reboot.tsx';

function Tray({status}: {status: any}) {
  return (
    <div id="tray">
      { (status?.battery || []).map((battery: any, idx: number) => (
        <div class="battery-level" style={ {"--perc": battery.reported_soc + '%'} } key={idx}>{ battery.reported_soc.toFixed(0) }%{
          battery.i > 0.05 ? <span>▲</span> : battery.i < -0.05 ? <span>▼</span> : ''
        }</div>
      )) }
      { status?.pause && <div class="badge" data-status="warn">PAUSED</div> }
    </div>
  );
}

function EventCount({status}: {status: any}) {
  const latest = window.latest_event_time || 0;
  const events = (status?.events || []).filter((ev: any) => ((status._now - ev.age) > (latest + 10)));
  const levels: {[key: string]: number} = {'ERROR': 2, 'WARNING': 1};
  const level = Math.max(...events.map((ev: any) => (levels[ev.level] || 0)), 0);
  const worst = level === 2 ? 'error' : level === 1 ? 'warn' : 'info';

  if(events.length===0) {
    return null;
  }
  return <span class="badge" data-status={worst}>{ events.length }</span>;
}

export function App() {
  const status = useGetApi("/api/status", 3000);
  const location = useLocation();

  function handlePause(ev: Event) {
    ev.preventDefault();

    fetch(import.meta.env.VITE_API_BASE + '/api/pause', {
        method: 'POST',
        body: ev.target.checked ? "1" : "0",
        mode: 'no-cors', // don't care about the response
    }).then(() => {
      // Trigger a data reload
      status?._reload();
    });
  }

  function handleEStop(ev: Event) {
    ev.preventDefault();
    if(!confirm(
      status?.estop 
      ? "This action will attempt to close contactors and enable power transfer. Are you sure?" : "This action will attempt to open contactors on the battery. Are you sure?"
    )) {
      return;
    }
    fetch(import.meta.env.VITE_API_BASE + '/api/estop', {
        method: 'POST',
        body: status?.estop ? "0" : "1",
    });
  }

  function handleReboot(ev: Event) {
    ev.preventDefault();
    if(!confirm("Are you sure you want to reboot the emulator?")) {
      return;
    }
    reboot();
    return new Promise<void>((_res, _rej) => {});
  }

  return (
    <>
      <div class="topbar">
        <h1>🔋 Battery Emulator</h1>
        <Tray status={status} />
        { status?.status && <Link href="/events"><div class="status" data-status={ status?.status }>{ status?.status }</div></Link> }
      </div>

      <div class="columns">
        <div class="menu">
          <div>
            <Link href="/" data-cur>Dashboard</Link>
            <Link href="/events">Events <EventCount status={status} /></Link>
            <Link href="/cellmonitor">Cell monitor</Link>
            <Link href="/extended">Extended info</Link>
            <Link href="/settings">Settings</Link>
            <Link href="/canlog">CAN log</Link>
            <Link href="/cansender">CAN sender</Link>
            <Link href="/log">System Log</Link>
            <Link href="/ota">OTA upgrade</Link>
            <a href="#" onClick={handlePause} class="button" style="margin: auto 0 0.75rem; background-color: #bf7c13; color: #ffffff;">{ status?.pause ? "Resume" : "Pause" }</a>
            <label class="toggle">
              <input type="checkbox" onChange={ handlePause } />
              Pause
            </label>
            <a href="#" onClick={handleEStop} class="button" style="margin: 0 0 0.75rem; background-color: #b50909; color: #ffffff;">Open contactors</a>
            <Button onClick={handleReboot} style={{"background-color": "#434343", "color": "#ffffff", "border-radius": "0 8px 8px 0"}}>Reboot emulator</Button>
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
          { location==="/" && <Dashboard status={status} /> }
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
