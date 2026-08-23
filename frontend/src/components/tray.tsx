//import { SocMeterLinear } from './components/socmeter.tsx'

import { Link } from '../utils/location.tsx';

export function Tray({status}: {status: any}) {
  return (
    <div id="tray">
      { /*
      { (status?.battery || []).map((battery: any, idx: number) => (
        <div class="battery-level" key={idx}>
          <SocMeterLinear soc={battery.reported_soc} one_hour_delta={battery.p * 100 / battery.reported_total_capacity} />
          <div class="battery-level-text">{ battery.reported_soc.toFixed(0) }%{
            battery.i > 0.05 ? <span>▲</span> : battery.i < -0.05 ? <span>▼</span> : ''
          }</div>
        </div>
      )) } 
      */ }
      { status?.estop && <div class="badge e s">ESTOP</div> }
      { status?.pause && <div class="badge d s">PAUSED</div> }
      { status?.status && <Link href="/events"><div class="badge s" data-status={ status?.status }>{ status?.status }</div></Link> }
    </div>
  );
}
