//import { createPortal } from 'preact/compat';

function sf(v: number, digits: number, unit?: string) {
  if(unit && v>=1000) {
    return sf(v/1000, digits, "k"+unit);
  }
  let ret = '' + v;
  for(let i=0; i<=digits; i++) {
    if(Math.abs(v) >= Math.pow(10, digits - i - 1)) {
      ret = v.toFixed(i);
      break;
    }
  }
  if(unit) {
    return (<>{ ret }<em>{unit}</em></>);
  }
  return ret;

}

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

const CONTACTOR_STATE_NAME = ["DISCONNECTED", "NEGATIVE CONNECTED", "PRECHARGING", "POSITIVE CONNECTED", "PRECHARGED", "CONNECTED", "SHUTDOWN REQUESTED"];
const CONTACTOR_STATE_STATUS = ["error", "warn", "warn", "ok", "ok", "ok", "error"];


export function Dashboard({status}: {status: any}) {
  if(!status) {
    return <div></div>;
  }
          
  //const tray = document.getElementById('tray');

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
      <div class="panel">
        <h3 data-status="warn">System</h3>
        <div class="stats">
          <div class="stat stat__small">
            <span>SOFTWARE</span>
            { status.firmware }
          </div>
          <div class="stat stat__small">
            <span>HARDWARE</span>
            { status.hardware }
          </div>
          <div class="stat stat__small">
            <span>CPU</span>
            { sf(status.temp, 3) }<em>°C</em>
          </div>
          <div class="stat stat__small">
            <span>UPTIME</span>
            { uptime(status.uptime) }
          </div>
        </div>
      </div>
      <div class="panel">
          <h3 data-status="ok">Network</h3>
          <div class="stats">
              { status.ssid && <>
                <div class="stat stat__small">
                    <span>SSID</span>
                    { status.ssid }
                </div>
                <div class="stat stat__small">
                    <span>RSSI</span>
                    { status.rssi }<em>dBm</em>
                </div>
                <div class="stat stat__small">
                    <span>HOSTNAME</span>
                    { status.hostname }
                </div>
                <div class="stat stat__small">
                    <span>IP</span>
                    { status.ip }
                </div>
              </> }
              { !!status.ap_ssid && <div class="stat stat__small">
                  <span>AP SSID</span>
                  { status.ap_ssid }
              </div> }
          </div>
      </div>

      { status.inverter && 
        <div class="panel">
            <h3 data-status="ok">Inverter &nbsp;<span>-&nbsp; { status.inverter.name }</span></h3>
        </div> }

      { status.contactor && 
        <div class="panel">
          <div class="badge" style="float: right" data-status={ CONTACTOR_STATE_STATUS[status.contactor.state] }>{ CONTACTOR_STATE_NAME[status.contactor.state] }</div>
          <h3 data-status={ CONTACTOR_STATE_STATUS[status.contactor.state] }>Contactors &nbsp;<span>-&nbsp; GPIO</span></h3>
        </div> }

      { (status.battery || []).map((battery: any, idx: number) =>
        <div class="panel" key={idx}>
            { battery.i > 0.05 ?
              <div class="badge" style="float: right; background: #1DB706; margin-left: 0.8rem">CHARGING</div>
              : battery.i < -0.05 ?
                <div class="badge" style="float: right; background: #138cd2; margin-left: 0.8rem">DISCHARGING</div>
                :
                <div class="badge" style="float: right; background: #3a516f; margin-left: 0.8rem">IDLE</div>
            }
            <div style="float: right; background: #3a516f; color: #ffffff; padding: 0.3rem 0.6rem 0.2rem; border-radius: 6px; font-size: 0.9rem; font-weight: 600">SCALED</div>
            <h3 data-status="ok">Battery { (status.battery?.length>1)?(idx+1):'' }&nbsp;<span>-&nbsp; { battery.protocol }</span></h3>

            <div class="stats">
                <div class="stat">
                    <span>SOC</span>
                    { sf(battery.reported_soc, 3)}<em>%</em>
                </div>
                <div class="stat">
                    <span>POWER</span>
                    { sf(battery.p, 3, 'W') }
                </div>
                <div class="stat">
                    <span>ENERGY REMAINING</span>
                    { sf(battery.reported_remaining_capacity/1000, 3) }<em>/</em> { sf(battery.reported_total_capacity/1000, 3) }<em>kWh</em>
                </div>
                <div class="stat">
                    <span>VOLTAGE</span>
                    { sf(battery.v, 4, 'V') }
                </div>
                <div class="stat">
                    <span>CURRENT</span>
                    { sf(battery.i, 2, 'A') }
                </div>
                
                { battery.temp_min < 10 &&
                  <div class="stat">
                    <span>MIN TEMP</span>
                    { sf(battery.temp_min, 3) }<em>°C</em>
                  </div> }
                { (battery.temp_min >= 10 || battery.temp_max >= 20) && 
                  <div class="stat">
                    <span>MAX TEMP</span>
                    { sf(battery.temp_max, 3) }<em>°C</em>
                  </div> }
                <div class="stat">
                    <span>CELL DELTA</span>
                    { battery.cell_mv_max - battery.cell_mv_min }<em>mV</em>
                </div>
            </div>
        </div>
      ) }


  </div>;
}
