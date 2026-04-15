import { useMemo } from "preact/hooks";

import { useGetApi } from "./utils/api.tsx";
import { 
    DATALAYER_INFO_BOLTAMPERA_FIELDS,
    DATALAYER_INFO_BMWPHEV_FIELDS,
    DATALAYER_INFO_BYDATTO3_FIELDS,
    DATALAYER_INFO_CELLPOWER_FIELDS,
    DATALAYER_INFO_CHADEMO_FIELDS,
    DATALAYER_INFO_CMFAEV_FIELDS,
    DATALAYER_INFO_ECMP_FIELDS,
    DATALAYER_INFO_GEELY_GEOMETRY_C_FIELDS,
    DATALAYER_INFO_KIAHYUNDAI64_FIELDS,
//    DATALAYER_INFO_TESLA_FIELDS,
    DATALAYER_INFO_NISSAN_LEAF_FIELDS,
    DATALAYER_INFO_MEB_FIELDS,
    DATALAYER_INFO_VOLVO_POLESTAR_FIELDS,
    DATALAYER_INFO_VOLVO_HYBRID_FIELDS,
    DATALAYER_INFO_ZOE_FIELDS,
    DATALAYER_INFO_ZOE_PH2_FIELDS,
} from "./ext/datalayer.ts";

import { Basic } from "./ext/basic.tsx";
import { TeslaExtended } from "./ext/tesla.tsx";

const FIELD_LISTS = {
    4: DATALAYER_INFO_BOLTAMPERA_FIELDS,
    43: DATALAYER_INFO_BMWPHEV_FIELDS,
    5: DATALAYER_INFO_BYDATTO3_FIELDS,
    6: DATALAYER_INFO_CELLPOWER_FIELDS,
    7: DATALAYER_INFO_CHADEMO_FIELDS,
    8: DATALAYER_INFO_CMFAEV_FIELDS,
    13: DATALAYER_INFO_ECMP_FIELDS,
    10: DATALAYER_INFO_GEELY_GEOMETRY_C_FIELDS,
    17: DATALAYER_INFO_KIAHYUNDAI64_FIELDS,
    // 32: DATALAYER_INFO_TESLA_FIELDS,
    // 33: DATALAYER_INFO_TESLA_FIELDS,
    21: DATALAYER_INFO_NISSAN_LEAF_FIELDS,
    19: DATALAYER_INFO_MEB_FIELDS,
    35: DATALAYER_INFO_VOLVO_POLESTAR_FIELDS,
    36: DATALAYER_INFO_VOLVO_HYBRID_FIELDS,
    28: DATALAYER_INFO_ZOE_FIELDS,
    29: DATALAYER_INFO_ZOE_PH2_FIELDS,
} as { [key: number]: any[] };

function command(id: string, cmd: string) {
    return () => {
        fetch(`/api/batteries/${id}/${cmd}`, { method: 'POST' }).then((r) => {
            if(r.status == 204) {
                alert(cmd + ' sent.');
            } else {
                alert('Error sending command: ' + r.statusText);
            }
        });
    };
}

export function Extended() {
    const data = useGetApi('/api/batext', 5000);
    const commands = useGetApi('/api/batteries/', 0);

    const old = useGetApi('/api/batold', 5000);

    console.log(commands);

    const view = useMemo(() => {
        if(!data) return null;
        const buf = new Uint8Array(data.length);
        for(let i=0; i<data.length; i++) {
            buf[i] = data.charCodeAt(i);
        }
        return new DataView(buf.buffer);
    }, [data]);
    
    const btype = view?.getUint32(0, true) || 0;
    const fields = FIELD_LISTS[btype] || [];

    return (
        <>
            <h2>Extended battery info</h2>

            <div dangerouslySetInnerHTML={{__html: old }}></div>
            <hr />

            { btype == 32 || btype == 33 ?
                (view && <TeslaExtended view={ view } />) :
                <Basic view={ view } fields={ fields } />
            }

            { commands?.battery?.map((bat: any) => (
                <div key={ bat.id }>
                    <h3>Battery { bat.id }</h3>
                    { bat.commands?.reset_bms && <button onClick={command(bat.id, 'reset_bms')}>Reset BMS</button> }&nbsp;
                    { bat.commands?.reset_dtc && <button onClick={command(bat.id, 'reset_dtc')}>Reset DTCs</button> }&nbsp;
                </div>
            )) }
        </>
    );
};
