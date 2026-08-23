import { useEffect, useRef, useState } from 'preact/hooks';

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
import { EventCount } from './components/event_count.tsx'
import { Tray } from './components/tray.tsx'

import { useGetApi, refreshApi } from './utils/api.tsx'
import { Link, useLocation } from './utils/location.tsx';
import { reboot } from './utils/reboot.tsx';
import { delay } from './utils/delay.ts';


export function App() {
  const status = useGetApi("/api/status", 3000);
  const location = useLocation();
  const [pauseChecked, setPauseChecked] = useState(false);
  const [menuOpen, setMenuOpen] = useState(false);
  const menuRef = useRef<HTMLDivElement>(null);
  useEffect(() => {
    setMenuOpen(false);
    menuRef.current?.scrollTo({top: 0, left: 0, behavior: 'auto'});
  }, [location]);

  function handlePause(ev: Event) {
    ev.preventDefault();
    
    const target = ev.target as HTMLInputElement | null;
    const shouldPause = target?.type === 'checkbox' ? target.checked : !status?.pause;

    fetch(import.meta.env.VITE_API_BASE + '/api/pause', {
        method: 'POST',
        body: shouldPause ? "1" : "0",
        mode: 'no-cors', // don't care about the response
    }).then(() => delay(500)).then(refreshApi);
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
    }).then(() => delay(500)).then(refreshApi);
  }

  function handleReboot(ev: Event) {
    ev.preventDefault();
    if(!confirm("Are you sure you want to reboot the emulator?")) {
      return;
    }
    reboot();
    return new Promise<void>((_res, _rej) => {});
  }

  useEffect(() => {
    setPauseChecked(!!status?.pause);
  }, [status?.pause]);

  return (
    <div data-menu-open={menuOpen ? "1" : undefined}>
      <div class="topbar">
        <h1 class="nm">🔋 Battery Emulator</h1>
        <Tray status={status} />
      </div>

      <div class="columns">
        <div class="menu-cloak" onClick={ () => setMenuOpen(false) }></div>
        <a href="#" class="menu-toggle" onClick={ () => setMenuOpen(!menuOpen) }>☰</a>
        <div class="menu" ref={menuRef}>
          <a href="#" class="menu-close mo" onClick={ () => setMenuOpen(false) }>✕</a>
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
            <label class="toggle gap-above" style="background-color: #bf7c13">
              <input type="checkbox" onChange={ handlePause } checked={ pauseChecked } />
              Pause
            </label>
            <a href="#" onClick={handleEStop} class="button" style="margin: 0 0 0.75rem; background-color: #b50909; color: #ffffff;">
              { status?.estop ? "Close contactors" : "Open contactors" }
            </a>
            <Button onClick={handleReboot} style={{"background-color": "#434343", "color": "#ffffff", "border-radius": "0 8px 8px 0"}}>Reboot emulator</Button>
          </div>
        </div>
        <div class="content">
          { location==="/canlog" && <CanLog /> }
          { location==="/cansender" && <CanSender /> }
          { location==="/cellmonitor" && <CellMonitor /> }
          { location==="/events" && <Events status={status} /> }
          { location==="/extended" && <Extended /> }
          { location==="/log" && <Log /> }
          { location==="/ota" && <Ota /> }
          { location==="/settings" && <Settings  /> }
          { location==="/" && <Dashboard status={status} /> }
        </div>
      </div>

    </div>
  );
}
