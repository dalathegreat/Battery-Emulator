//import { createPortal } from 'preact/compat';
import { PowerMeter } from './components/power_meter';
import { sf } from "./utils/formatting";

function uptime(s: number) {
  s = (s / 1000);
  const seconds = (s % 60).toFixed(0);
  s = Math.floor(s / 60);
  const minutes = (s % 60).toFixed(0);
  s = Math.floor(s / 60);
  const hours = (s % 24).toFixed(0);
  s = Math.floor(s / 24);
  const days = (s).toFixed(0);
  return (
    <>{days}<u>d</u> {hours}<u>h</u> {minutes}<u>m</u> {seconds}<u>s</u></>
  );
}

function SocMeter({soc, one_hour_delta}: { soc: number, one_hour_delta: number }) {
  let arc = (
    <circle cx="50" cy="50" r="48" pathLength={100} style={{ strokeDasharray: `${soc} 100` }} />
  );
  let delta = Math.abs(one_hour_delta) > 1 ? (
    <circle cx="50" cy="50" r="46" pathLength={100} class="delta" style={{
      stroke: one_hour_delta > 0 ? '#138cd2' : '#e7ce11ff',
      strokeDasharray: `${Math.abs(one_hour_delta)} 100`, 
      strokeDashoffset: -soc - (one_hour_delta < 0 ? one_hour_delta : 0),
      strokeWidth: 3,
      opacity: 1
    }} />
  ) : '';
  return <svg viewBox="0 0 100 100">
    <circle cx="50" cy="50" r="48" pathLength={100} class="base" />
    { one_hour_delta < 0 ?
      // Delta overlaps the arc
      <>{arc}{delta}</>
      :
      // Arc overlaps the delta
      <>{delta}{arc}</>
    }
  </svg>
}


const CONTACTOR_STATE_NAME = ["DISCONNECTED", "NEGATIVE CONNECTED", "PRECHARGING", "POSITIVE CONNECTED", "PRECHARGED", "CONNECTED", "SHUTDOWN REQUESTED"];
const CONTACTOR_STATE_STATUS = ["error", "warn", "warn", "ok", "ok", "ok", "error"];

export function Dashboard({ status }: { status: any }) {
  if (!status) {
    return <div></div>;
  }

  //const tray = document.getElementById('tray');
  //status.battery[0].p = -1000;
  //status.battery[0].discharge_p_max = 2000;

  return <div>
    { /*tray && createPortal(
        ((data.battery || []).map((battery: any, idx: number) => (
          <div class="battery-level" style={ {"--perc": battery.reported_soc + '%'} } key={idx}>{ sf(battery.reported_soc, 0) }%{
            battery.i > 0.05 ? <span>▲</span> : battery.i < -0.05 ? <span>▼</span> : ''
          }</div>
        ))),
        tray
    ) */}
    <h2>Dashboard</h2>



    {status.contactor &&
      <div class="panel" data-status={CONTACTOR_STATE_STATUS[status.contactor.state]}>
        <div class="badge" style="float: right" data-status={CONTACTOR_STATE_STATUS[status.contactor.state]}>{CONTACTOR_STATE_NAME[status.contactor.state]}</div>
        <h3>Contactors &nbsp;<span>-&nbsp; GPIO</span></h3>
      </div>}

    {(status.battery || []).map((battery: any, idx: number) =>
      <div class="panel" key={idx} data-status={battery.status?.toLowerCase()}>
        {battery.i > 0.05 ?
          <div class="badge" style="float: right; background: #138cd2; margin-left: 0.8rem">CHARGING</div>
          : battery.i < -0.05 ?
            <div class="badge" style="float: right; background: #e7ce11; margin-left: 0.8rem">DISCHARGING</div>
            :
            <div class="badge" style="float: right; background: #3a516f; margin-left: 0.8rem">IDLE</div>
        }
        <div style="float: right; background: #3a516f; color: #ffffff; padding: 0.3rem 0.6rem 0.2rem; border-radius: 6px; font-size: 0.9rem; font-weight: 600">SCALED</div>
        <h3>Battery {(status.battery?.length > 1) ? (idx + 1) : ''}&nbsp;<span>-&nbsp; {battery.protocol}</span></h3>

        <div class="stats">
          <div class="stat-circle">
            <SocMeter soc={battery.reported_soc} one_hour_delta={battery.p * 100 / battery.reported_total_capacity} />
            <div>{sf(battery.reported_soc, 3)}<br /><em>%</em></div>
          </div>
          {/* <div class="stat">
            <span>POWER</span>
            {sf(battery.p, 3, 'W')}
          </div> */}
          <div class="stats">
          
          <div class="stat">
            <span>POWER</span>
            <PowerMeter p={battery.p} discharge_p_max={battery.discharge_p_max} charge_p_max={battery.charge_p_max} unit="W" />
          </div>

          <div class="stat">
            <span>ENERGY REMAINING</span>
            {sf(battery.reported_remaining_capacity / 1000, 3)}<em>/</em> {sf(battery.reported_total_capacity / 1000, 3)}<em>kWh</em>
          </div>
          <div class="stat">
            <span>VOLTAGE</span>
            {sf(battery.v, 4, 'V')}
            <em>123.3V 🡘 222.2V</em>
          </div>
          <div class="stat">
            <span>CURRENT</span>
            <PowerMeter p={battery.i} discharge_p_max={battery.discharge_i_max} charge_p_max={battery.charge_i_max} unit="A" />
          </div>

          {battery.temp_min < 10 &&
            <div class="stat">
              <span>MIN TEMP</span>
              {sf(battery.temp_min, 3)}<em>°C</em>
            </div>}
          {(battery.temp_min >= 10 || battery.temp_max >= 20) &&
            <div class="stat">
              <span>MAX TEMP</span>
              {sf(battery.temp_max, 3)}<em>°C</em>
            </div>}
          <div class="stat">
            <span>CELL DELTA</span>
            {battery.cell_mv_max - battery.cell_mv_min}<em>mV</em>
          </div>
          </div>
        </div>
      </div>
    )}

    {status.inverter &&
      <div class="panel" data-status={status.inverter.status?.toLowerCase()}>
        { status.inverter.status == 'INACTIVE' && 
          <div style="float: right; background: #3a516f; color: #ffffff; padding: 0.3rem 0.6rem 0.2rem; border-radius: 6px; font-size: 0.9rem; font-weight: 600">IDLE</div>
        }
        <h3>Inverter &nbsp;<span>-&nbsp; {status.inverter.name}</span></h3>
      </div>}    

    <div class="panel" data-status="ok">
      <h3>Network</h3>
      <div class="stats">
        {status.ssid && <>
          <div class="stat stat__small">
            <span>SSID</span>
            {status.ssid}
          </div>
          <div class="stat stat__small">
            <span>RSSI</span>
            {status.rssi}<em>dBm</em>
          </div>
          <div class="stat stat__small">
            <span>HOSTNAME</span>
            {status.hostname}
          </div>
          <div class="stat stat__small">
            <span>IP</span>
            {status.ip}
          </div>
        </>}
        {!!status.ap_ssid && <div class="stat stat__small">
          <span>AP SSID</span>
          {status.ap_ssid}
        </div>}
      </div>
    </div>

    <div class="panel" data-status="ok">
      <h3>System</h3>
      <div class="stats">
        <div class="stat stat__small">
          <span>SOFTWARE</span>
          {status.firmware}
        </div>
        <div class="stat stat__small">
          <span>HARDWARE</span>
          {status.hardware}
        </div>
        <div class="stat stat__small">
          <span>CPU</span>
          {sf(status.temp, 3)}<em>°C</em>
        </div>
        <div class="stat stat__small">
          <span>UPTIME</span>
          {uptime(status.uptime)}
        </div>
      </div>
    </div>

  </div>;
}
