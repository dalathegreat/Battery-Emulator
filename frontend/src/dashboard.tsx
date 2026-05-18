//import { createPortal } from 'preact/compat';

function sf(v: number, digits: number, unit?: string) {
  if (unit && v >= 1000) {
    return sf(v / 1000, digits, "k" + unit);
  }
  let ret = '' + v;
  for (let i = 0; i <= digits; i++) {
    if (Math.abs(v) >= Math.pow(10, digits - i - 1)) {
      ret = v.toFixed(i);
      break;
    }
  }
  if (unit) {
    return (<>{ret}<em>{unit}</em></>);
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

function SocMeter({soc, one_hour_delta}: { soc: number, one_hour_delta: number }) {
  let arc = (
    <circle cx="50" cy="50" r="46" pathLength={100} style={{ strokeDasharray: `${soc} 100` }} />
  );
  let delta = (
    <circle cx="50" cy="50" r="46" pathLength={100} class="delta" style={{
      stroke: one_hour_delta > 0 ? '#138cd2' : '#fbe81bff',
      strokeDasharray: `${Math.abs(one_hour_delta)} 100`, 
      strokeDashoffset: -soc - (one_hour_delta < 0 ? one_hour_delta : 0),
      opacity: 0.5 
    }} />
  );
  return <svg>
    <circle cx="50" cy="50" r="46" pathLength={100} class="base" />
    { one_hour_delta < 0 ?
      // Delta overlaps the arc
      <>{arc}{delta}</>
      :
      // Arc overlaps the delta
      <>{delta}{arc}</>
    }
  </svg>
}

function PowerMeter1({ p, discharge_p_max, charge_p_max }: { p: number, discharge_p_max: number, charge_p_max: number }) {
  return <div>
            <div style="
              position: relative;
              width: 150px; height: 30px; margin-top: 0.5rem;
            ">

              <div style={`
                width: ${Math.min(100, Math.max(0, discharge_p_max / 10000 * 100))}%;
                position: absolute;
                right: 50%;
                top: 70%;
                height: 30%;
                background: #ffffff33;
              `}></div>
              <div style={`
                position: absolute;
                font-size: 0.7rem;
                left: 0;
              `}>-{ sf(discharge_p_max, 3, 'W') }</div>
              <div style={`
                position: absolute;
                font-size: 0.7rem;
                right: 0;
              `}>{ sf(charge_p_max, 3, 'W') }</div>
              <div style={`
                width: ${Math.min(100, Math.max(0, charge_p_max / 10000 * 100))}%;
                position: absolute;
                left: 50%;
                top: 70%;
                height: 30%;
                background: #ffffff33;
              `}></div>
              <div style={`
                width: ${Math.min(100, Math.max(0, p / 10000 * 100))}%;
                position: absolute;
                left: 50%;
                top: 70%;
                height: 30%;
                background: ${p > 0 ? '#138cd2' : p < 0 ? '#fbe81bff' : '#3a516f'};
              `}></div>
              <div style={`
                position: absolute;
                width: 100%;
                top: 70%;
                height: 30%;
                border: 1px solid #555555; border-radius: 2px; 
              `}></div>
              <div style={`
                position: absolute;
                width: 1px;
                left: 50%;
                top: 70%;
                height: 30%;
                background: #cccccc;
              `}></div>

            </div>
          </div>
}
PowerMeter1;

function PowerMeter2({ p, discharge_p_max, charge_p_max }: { p: number, discharge_p_max: number, charge_p_max: number }) {
  //charge_p_max = 0;

  var percent_per_watt = 50 / Math.max(discharge_p_max, charge_p_max);
  var offset = 100 * (discharge_p_max / (discharge_p_max + charge_p_max));


  // const cpm = charge_p_max;//Math.max(charge_p_max, 1000);
  // const dpm = discharge_p_max;//Math.max(discharge_p_max, 1000);
  // var percent_per_watt = 100 / (dpm + cpm);
  // var offset = 100 * (dpm / (dpm + cpm));
                // left: calc(5% - 1px);
                // right: calc(5% - 1px);
                // left: calc(${50 - (discharge_p_max * percent_per_watt)}% - 1px);
                // right: calc(${50 - (charge_p_max * percent_per_watt)}% - 1px);

  return <div>
            <div style="
              position: relative;
              width: 150px; height: 30px;
            ">
              <div style={`
                position: absolute;
                height: 21px;
                left: calc(${50 - (discharge_p_max * percent_per_watt)}% - 1px);
                right: calc(${50 - (charge_p_max * percent_per_watt)}% - 1px);
                border-bottom: 1px solid #fff;
              }`}>
              <div style={`
                position: absolute;
                font-size: 0.7rem;
                line-height: 1;
                left: -5px;
              `}>-{ +(discharge_p_max/1000).toFixed(1) + 'kW' }</div>
              <div style={`
                position: absolute;
                font-size: 0.7rem;
                line-height: 1;
                right: 0;
              `}>{ +(charge_p_max/1000).toFixed(1) + 'kW' }</div>
              <div style={`
                position: absolute;
                left: ${offset - Math.max(0, -p * percent_per_watt)}%;
                right: ${100 - (offset + Math.max(0, p * percent_per_watt))}%;
                bottom: 0px;
                height: 6px;
                background: ${p > charge_p_max ? '#bc0000ff' :
                              p < -discharge_p_max ? '#bc0000ff' :
                              p > 0 ? '#138cd2' : '#fbe81bff'};
              `}></div>
              <div style={`
                position: absolute;
                left: 0px;
                height: 5px;
                width: 1px;
                bottom: 0px;
                border-left: 1px solid #ffffff;
              `}></div>
              <div style={`
                position: absolute;
                right: 0;
                height: 5px;
                width: 1px;
                bottom: 0px;
                border-right: 1px solid #ffffff;
              `}></div>
              <div style={`
                position: absolute;
                left: calc(${offset + (p * percent_per_watt)}% - 5px);
                height: 5px;
                width: 5px;
                bottom: -6px;
                border: 5px solid #0000;
                border-bottom: 5px solid #ffffff;
              `}></div>
              <div style={`
                position: absolute;
                width: 1px;
                left: calc(${offset}% - 1px);
                height: 9px;
                bottom: 0px;
                background: #ffffff;
              `}></div>
              <div style={`
                position: absolute;
                width: 1px;
                width: 50px;
                text-align: center;
                left: calc(${offset + (p * percent_per_watt)}% - 25px);
                font-size: 0.8rem;
                font-weight: 600;
                bottom: -27px;
              `}>{ sf(p, 3, 'W') }</div>

              </div>
            </div>
          </div>;
}

function sf_str(v: number, digits: number): string {
  for (let i = 0; i <= digits; i++) {
    if (Math.abs(v) >= Math.pow(10, digits - i - 1)) {
      return v.toFixed(i);
    }
  }
  return '' + v;
}

function PowerMeter2canvas({ p, discharge_p_max, charge_p_max }: { p: number, discharge_p_max: number, charge_p_max: number }) {
  const width = 150, height = 30, center = width / 2;
  const max = Math.max(discharge_p_max, charge_p_max, 1);
  const scale = (width / 2) / max;

  return <canvas width={width} height={height + 15} ref={el => {
    if (!el) return;
    const ctx = el.getContext('2d')!;
    ctx.clearRect(0, 0, width, height + 15);
    ctx.font = '10px Inter, sans-serif';
    ctx.fillStyle = ctx.strokeStyle = '#fff';

    // Scales & Limits
    ctx.fillText(`-${sf_str(discharge_p_max, 3)}W`, 0, 10);
    ctx.textAlign = 'right';
    ctx.fillText(`${sf_str(charge_p_max, 3)}W`, width, 10);
    
    // Axis
    ctx.beginPath();
    ctx.moveTo(center - discharge_p_max * scale, 25);
    ctx.lineTo(center + charge_p_max * scale, 25);
    ctx.stroke();

    // Ticks
    [center - discharge_p_max * scale, center + charge_p_max * scale, center].forEach(x => {
      ctx.beginPath(); ctx.moveTo(x, 20); ctx.lineTo(x, 25); ctx.stroke();
    });

    // Power Bar
    ctx.fillStyle = p > 0 ? '#138cd2' : p < 0 ? '#fbe81b' : '#3a516f';
    ctx.fillRect(center, 20, p * scale, 5);

    // Indicator Triangle & Text
    const px = center + p * scale;
    ctx.fillStyle = '#fff';
    ctx.beginPath();
    ctx.moveTo(px, 30); ctx.lineTo(px - 4, 35); ctx.lineTo(px + 4, 35);
    ctx.fill();
    ctx.textAlign = 'center';
    ctx.fillText(`${sf_str(p, 3)}W`, px, 43);
  }} />;
}
PowerMeter2canvas;

const CONTACTOR_STATE_NAME = ["DISCONNECTED", "NEGATIVE CONNECTED", "PRECHARGING", "POSITIVE CONNECTED", "PRECHARGED", "CONNECTED", "SHUTDOWN REQUESTED"];
const CONTACTOR_STATE_STATUS = ["error", "warn", "warn", "ok", "ok", "ok", "error"];

export function Dashboard({ status }: { status: any }) {
  if (!status) {
    return <div></div>;
  }

  //const tray = document.getElementById('tray');
  //status.battery[0].p = -1000;
  status.battery[0].discharge_p_max = 2000;

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

    {status.inverter &&
      <div class="panel" data-status={status.inverter.status?.toLowerCase()}>
        { status.inverter.status == 'INACTIVE' && 
          <div style="float: right; background: #3a516f; color: #ffffff; padding: 0.3rem 0.6rem 0.2rem; border-radius: 6px; font-size: 0.9rem; font-weight: 600">IDLE</div>
        }
        <h3>Inverter &nbsp;<span>-&nbsp; {status.inverter.name}</span></h3>
      </div>}

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
            <div class="badge" style="float: right; background: #fbe81bff; margin-left: 0.8rem">DISCHARGING</div>
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
          
          <div class="stat">
            <span>POWER</span>
            <PowerMeter2 p={battery.p} discharge_p_max={battery.discharge_p_max} charge_p_max={battery.charge_p_max} />
          </div>

          <div class="stat">
            <span>ENERGY REMAINING</span>
            {sf(battery.reported_remaining_capacity / 1000, 3)}<em>/</em> {sf(battery.reported_total_capacity / 1000, 3)}<em>kWh</em>
          </div>
          <div class="stat">
            <span>VOLTAGE</span>
            {sf(battery.v, 4, 'V')}
          </div>
          <div class="stat">
            <span>CURRENT</span>
            {sf(battery.i, 2, 'A')}
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
    )}


  </div>;
}
