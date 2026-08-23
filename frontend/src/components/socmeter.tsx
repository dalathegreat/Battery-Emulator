// SoC meter control: circular (dashboard) and linear (topbar tray) variants.
// Both take the reported SoC (0-100) and the one-hour delta (in % SoC),
// i.e. battery.p * 100 / battery.reported_total_capacity.

export function SocMeter({soc, one_hour_delta}: { soc: number, one_hour_delta: number }) {
  let arc = (
    <circle cx="50" cy="50" r="48" pathLength={100} style={{ strokeDasharray: `${soc} 100` }} />
  );
  let delta = Math.abs(one_hour_delta) > 1 ? (
    <circle cx="50" cy="50" r="44" pathLength={100} class="delta" style={{
      stroke: one_hour_delta > 0 ? '#138cd2' : '#e7ce11ff',
      strokeDasharray: `${Math.abs(one_hour_delta)} 100`, 
      strokeDashoffset: -soc - (one_hour_delta < 0 ? one_hour_delta : 0),
      strokeWidth: 4,
      opacity: 1
    }} />
  ) : '';
  // Small white radial tick at the end of the SoC arc (where the delta arc starts).
  // The SVG is rotated -90deg in CSS so the start shows at 12 o'clock; rotate()
  // positions the tick (a static outward line at r=45..50) at the arc end.
  let tick = (
    <line
      x1="90" y1="50" x2="100" y2="50"
      stroke="#fff" stroke-width="1.5"
      transform={`rotate(${soc * 3.6} 50 50)`}
    />
  );
  return <svg viewBox="0 0 100 100">
    <circle cx="50" cy="50" r="48" class="base" />
    {/* Fixed order (arc first): a negative delta's yellow tail overlaps the end
        of the green arc, a positive delta never overlaps it. Keeping the order
        constant lets CSS transitions animate sign flips instead of remounting. */}
    {arc}{delta}
    {tick}
  </svg>
}

// Linear variant: horizontal bar that mirrors the circular meter.
// Green = SoC fill, blue = positive delta (extends right), yellow = negative
// delta (covers the tail of the green fill, i.e. the SoC predicted to be
// consumed), white tick at the end of the SoC fill where the delta starts.
export function SocMeterLinear({soc, one_hour_delta}: { soc: number, one_hour_delta: number }) {
  let d = Math.abs(one_hour_delta) > 1 ? one_hour_delta : 0;
  return (
    <svg class="socmeter" viewBox="0 0 100 16">
      <rect class="base" x="0" y="0" width="100" height="16" rx="8" />
      { d > 0 && <rect fill="#138cd2" x={soc} y="0" width={d} height="16" rx="2" /> }
      <rect fill="#1DB706" x="0" y="0" width={soc} height="16" rx="2" />
      { d < 0 && <rect fill="#e7ce11" x={soc + d} y="0" width={-d} height="16" rx="2" /> }
      <line x1="0" y1="0" x2="0" y2="16" stroke="#fff" strokeWidth="2" transform={`translate(${soc})`} />
    </svg>
  );
}
