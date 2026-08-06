import * as L from "./datalayer/MgGen1Battery.ts";
import { lookup } from "./consts.ts";

export function MgGen1Extended({view}: {view: DataView}) {
    const get = lookup(view);

    // Printable ASCII (32..126) as text, anything else as [hex],
    // matching MgGen1Battery::print_chars_or_hex.
    function charsOrHex(bytes: number[]): string {
        return bytes.map(b => b >= 32 && b <= 126 ? String.fromCharCode(b) : `[${b.toString(16).padStart(2, '0')}]`).join('');
    }
    function hexByte(b: number): string {
        return b.toString(16).padStart(2, '0').toUpperCase();
    }

    const vin = get( L.pid_vin) as number[];
    const mfrDate = get( L.pid_mfr_date) as number[];
    const fingerprint = get( L.pid_fingerprint) as number[];
    const vehicleHwNo = get( L.pid_vehicle_hw_number) as number[];
    const systemHwNo = get( L.pid_system_hw_number) as number[];
    const systemSwNo = get( L.pid_system_sw_number) as number[];
    const f18a = get( L.pid_f18a) as number[];
    const f120 = get( L.pid_f120) as number[];
    const b18c = get( L.pid_b18c) as number[];
    const f1a2 = get( L.pid_f1a2) as number[];
    const f1aa = get( L.pid_f1aa) as number[];

    return <div>
        UDS address: { (get( L._uds_address) as number).toString(16) }<br/>
        VIN: { charsOrHex(vin) }<br/>
        MfrDate: { `20${hexByte(mfrDate[0])}-${hexByte(mfrDate[1])}-${hexByte(mfrDate[2])}` }<br/>
        Fingerprint: { charsOrHex(fingerprint) }<br/>
        VehHWNo: { charsOrHex(vehicleHwNo) }<br/>
        SysHWNo: { charsOrHex(systemHwNo) }<br/>
        SysSWNo: { charsOrHex(systemSwNo) }<br/>
        F18A: { charsOrHex(f18a) }<br/>
        F120: { charsOrHex(f120) }<br/>
        B18C: { charsOrHex(b18c) }<br/>
        F1A2: { charsOrHex(f1a2) }<br/>
        F1AA: { charsOrHex(f1aa) }<br/>
    </div>;
};
