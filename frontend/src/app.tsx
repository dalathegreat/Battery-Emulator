// import { /*Link, */Route, Switch } from "wouter";


import { CanLog } from './can_log.tsx'
import { CellMonitor } from './cellmonitor.tsx'
import { Dashboard } from './dashboard.tsx'
import { Events } from './events.tsx'
import { Extended } from './extended.tsx'
import { Log } from './log.tsx'
import { Ota } from './ota.tsx'
import { Settings } from './settings.tsx'

import { Link, useLocation } from './utils/location.tsx';

export function App() {
  let location = useLocation();
  return (
    <>
      <div class="topbar">
        <h1>🔋 Battery Emulator</h1>
        <div id="tray"></div>
        <div class="status">SYSTEM OK</div>
      </div>

      <div class="columns">
        <div class="menu">
          <div>
            <Link href="/" data-cur>Dashboard</Link>
            <Link href="/events">Events <span class="badge">3</span></Link>
            <Link href="/cellmonitor">Cell monitor</Link>
            <Link href="/extended">Extended info</Link>
            <Link href="/settings">Settings</Link>
            <Link href="/canlog">CAN log</Link>
            <Link href="/log">System Log</Link>
            <Link href="/ota">OTA upgrade</Link>
            <a href="#" class="button" style="margin: auto 0 0.75rem; background-color: #bf7c13; color: #ffffff;">Pause</a>
            <a href="#" class="button" style="background-color: #b50909; color: #ffffff;">Open contactors</a>
          </div>
        </div>
        <div class="content">
          { location==="/canlog" && <CanLog /> }
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
