import { DATALAYER_INFO_TESLA } from "./datalayer.ts";
import { FUNCS, LENGTHS } from "./consts.ts";

export function TeslaExtended({view}: {view: DataView}) {
    function lookup(field: [number, string] | [number, string, number]) {
        if(field.length === 3) {
            const arr = [];
            for(let i=0; i<field[2]; i++) {
                arr.push((view as any)[FUNCS[field[1]] as keyof DataView](field[0] + i * LENGTHS[field[1]], true));
            }
            return arr;
        }
        return ((view as any)[FUNCS[field[1]] as keyof DataView] as any)(field[0], true);
        //return Reflect.get(view, FUNCS[field[1]]).call(view, field[0], true);
    }
    const L = DATALAYER_INFO_TESLA;


    return <div>
        LV Bus Voltage: { lookup(L.battery_dcdcLvBusVolt) } V<br/>
        Ser: { lookup(L.battery_serialNumber) }<br/>
    </div>;
};