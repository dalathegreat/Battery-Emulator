export function sf(v: number, digits: number, unit?: string) {
  if (unit && Math.abs(v) >= 1000) {
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
