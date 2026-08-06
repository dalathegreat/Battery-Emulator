import { sf } from "../utils/formatting";

/*
export function oldPowerMeter({ p, discharge_p_max, charge_p_max, unit }: { p: number, discharge_p_max: number, charge_p_max: number, unit: string }) {
  var percent_per_watt = 50 / Math.max(discharge_p_max, charge_p_max, 1);
  var inner_percent_per_watt = 100 / Math.max(discharge_p_max + charge_p_max, 2);
  if(discharge_p_max == 0 && charge_p_max == 0 && p != 0) {
    // Make it shoot past the end by a fixed amount
    inner_percent_per_watt = 300 / Math.abs(p);
  }
  var offset = 100 * (Math.max(discharge_p_max, 1) / Math.max(discharge_p_max + charge_p_max, 2));
  var over = p > charge_p_max || p < -discharge_p_max;

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
              width: 150px; height: 30px; margin: 0 9px 0 8px;
            ">
              <div style={`
                position: absolute;
                height: 21px;
                left: calc(min(${50 - (discharge_p_max * percent_per_watt)}%, 43%) - 1px);
                right: calc(min(${50 - (charge_p_max * percent_per_watt)}%, 43%) - 1px);
                border-bottom: 1px solid #fff;
              }`}>
              <div style={`
                position: absolute;
                font-size: 0.7rem;
                line-height: 1;
                left: -7px;
              `}>{ sf(-discharge_p_max, 2, unit) }</div>
              <div style={`
                position: absolute;
                font-size: 0.7rem;
                line-height: 1;
                right: -8px;
              `}>{ sf(charge_p_max, 2, unit) }</div>
              <div style={`
                position: absolute;
                left: ${offset - Math.max(0, -p * inner_percent_per_watt)}%;
                right: ${100 - (offset + Math.max(0, p * inner_percent_per_watt))}%;
                bottom: 0px;
                height: 6px;
                background: ${over ? '#bc0000ff' :
                              p > 0 ? '#138cd2' : '#e7ce11'};
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
                left: calc(${offset + (p * inner_percent_per_watt)}% - 5px);
                height: 5px;
                width: 5px;
                bottom: -6px;
                border: 5px solid #0000;
                border-bottom: 5px solid #ffffff;
              `}></div>
              <div style={`
                position: absolute;
                width: 1px;
                left: max(${offset}% - 1px, 0px);
                height: 9px;
                bottom: 0px;
                background: #ffffff;
              `}></div>
              <div style={`
                position: absolute;
                width: 1px;
                width: 50px;
                text-align: center;
                left: max(min(${offset + (p * inner_percent_per_watt)}%, 100%) - 25px, 0px);
                font-size: 1.3rem;
                font-weight: 500;
                bottom: -32px;
                color: ${over ? '#ee0000ff' : 'inherit'};
              `}>{ sf(p, 2, unit) }</div>

              </div>
            </div>
          </div>;
}
*/

interface PowerMeterProps {
  p: number;
  discharge_p_max: number;
  charge_p_max: number;
  unit: string;
}

export function PowerMeter({ p, discharge_p_max, charge_p_max, unit }: PowerMeterProps) {
  //p = p * 100;
  const maxLimit = Math.max(discharge_p_max, charge_p_max, 1);
  const totalLimit = Math.max(discharge_p_max + charge_p_max, 2);

  const percentPerWatt = 50 / maxLimit;
  let innerPercentPerWatt = 100 / totalLimit;

  if (discharge_p_max === 0 && charge_p_max === 0 && p !== 0) {
    innerPercentPerWatt = 300 / Math.abs(p);
  }

  const offset = 100 * (Math.max(discharge_p_max, 1) / totalLimit);
  const isOverLimit = p > (charge_p_max*1.1) || p < -(discharge_p_max*1.1);
  const currentPos = offset + p * innerPercentPerWatt;

  // Dynamic Style Calculations
  const axisStyle = {
    left: `calc(min(${50 - discharge_p_max * percentPerWatt}%, 43%) - 1px)`,
    right: `calc(min(${50 - charge_p_max * percentPerWatt}%, 43%) - 1px)`,
  };

  const barStyle = {
    left: `${offset - Math.max(0, -p * innerPercentPerWatt)}%`,
    right: `${100 - (offset + Math.max(0, p * innerPercentPerWatt))}%`,
    background: isOverLimit ? '#bc0000ff' : p > 0 ? '#138cd2' : '#e7ce11',
  };

  return (
    <div>
      <div className="pm-container">
        <div className="pm-axis" style={axisStyle}>
          {/* Labels */}
          <div className="pm-label pm-left">{sf(-discharge_p_max, 2, unit)}</div>
          <div className="pm-label pm-right">{sf(charge_p_max, 2, unit)}</div>

          {/* Dynamic Value Bar */}
          <div className="pm-bar" style={barStyle} />

          {/* Outer Ticks */}
          <div className="pm-tick pm-left" />
          <div className="pm-tick pm-right" />

          {/* Pointer & Center Marker */}
          <div className="pm-pointer" style={{ left: `calc(${currentPos}% - 5px)` }} />
          <div className="pm-center" style={{ left: `max(${offset}% - 1px, 0px)` }} />

          {/* Current Value Display */}
          <div 
            className="pm-value" 
            style={{ 
              left: `max(min(${currentPos}%, calc(100% - 20px)) - 25px, -12px)`,
              color: isOverLimit ? '#ee0000ff' : 'inherit'
            }}
          >
            {sf(p, 2, unit)}
          </div>
        </div>
      </div>
    </div>
  );
}
