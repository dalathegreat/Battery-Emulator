import { useMemo, useEffect, useRef } from "preact/hooks";

import { useGetApi } from "./utils/api.tsx";
// import { 
//     DATALAYER_INFO_BOLTAMPERA_FIELDS,
//     DATALAYER_INFO_BMWPHEV_FIELDS,
//     DATALAYER_INFO_BYDATTO3_FIELDS,
//     DATALAYER_INFO_CELLPOWER_FIELDS,
//     DATALAYER_INFO_CHADEMO_FIELDS,
//     DATALAYER_INFO_CMFAEV_FIELDS,
//     DATALAYER_INFO_ECMP_FIELDS,
//     DATALAYER_INFO_GEELY_GEOMETRY_C_FIELDS,
//     DATALAYER_INFO_KIAHYUNDAI64_FIELDS,
// //    DATALAYER_INFO_TESLA_FIELDS,
//     DATALAYER_INFO_NISSAN_LEAF_FIELDS,
//     DATALAYER_INFO_MEB_FIELDS,
//     DATALAYER_INFO_VOLVO_POLESTAR_FIELDS,
//     DATALAYER_INFO_VOLVO_HYBRID_FIELDS,
//     DATALAYER_INFO_ZOE_FIELDS,
//     DATALAYER_INFO_ZOE_PH2_FIELDS,
// } from "./ext/datalayer.ts";

import type { ComponentType } from "preact";

//import { Basic } from "./ext/basic.tsx";
import { TeslaExtended } from "./ext/tesla.tsx";
import { MgGen1Extended } from "./ext/mg.tsx";
import { BydAtto3Extended } from "./ext/byd_atto3.tsx";

// Battery-specific extended-info views. Keyed by the C++ BatteryType enum
// value (Software/src/battery/Battery.h) - keep in sync with it.
//
// Each battery with bespoke UI gets its own file under src/ext/ (byd_atto3,
// tesla, mg...). Batteries without an entry fall back to the generic Basic
// dump, driven by the generated DATALAYER_INFO_* field tables.
type ExtProps = { view: DataView; battery?: 1 | 2; cells?: number };
const BATTERY_EXT: Record<number, ComponentType<ExtProps>> = {
    5: BydAtto3Extended,  // BYD Atto 3 / Seal / Dolphin
    32: TeslaExtended,    // Tesla Model 3/Y
    33: TeslaExtended,    // Tesla Model S/X
    37: MgGen1Extended,   // MG HS PHEV (Gen1, UDS)
};

//const FIELD_LISTS = {
    // 4: DATALAYER_INFO_BOLTAMPERA_FIELDS,
    // 43: DATALAYER_INFO_BMWPHEV_FIELDS,
    // 5: DATALAYER_INFO_BYDATTO3_FIELDS,
    // 6: DATALAYER_INFO_CELLPOWER_FIELDS,
    // 7: DATALAYER_INFO_CHADEMO_FIELDS,
    // 8: DATALAYER_INFO_CMFAEV_FIELDS,
    // 13: DATALAYER_INFO_ECMP_FIELDS,
    // 10: DATALAYER_INFO_GEELY_GEOMETRY_C_FIELDS,
    // 17: DATALAYER_INFO_KIAHYUNDAI64_FIELDS,
    // // 32: DATALAYER_INFO_TESLA_FIELDS,
    // // 33: DATALAYER_INFO_TESLA_FIELDS,
    // 21: DATALAYER_INFO_NISSAN_LEAF_FIELDS,
    // 19: DATALAYER_INFO_MEB_FIELDS,
    // 35: DATALAYER_INFO_VOLVO_POLESTAR_FIELDS,
    // 36: DATALAYER_INFO_VOLVO_HYBRID_FIELDS,
    // 28: DATALAYER_INFO_ZOE_FIELDS,
    // 29: DATALAYER_INFO_ZOE_PH2_FIELDS,
//} as { [key: number]: any[] };

function command(id: string, cmd: string) {
    return () => {
        fetch((import.meta.env.VITE_API_BASE || '') + `/api/batteries/${id}/${cmd}`, { method: 'POST' }).then((r) => {
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
    const oldContainerRef = useRef<HTMLDivElement>(null);

    // Execute <script> tags in the old HTML
    useEffect(() => {
        if (oldContainerRef.current && old) {
            const scripts = oldContainerRef.current.querySelectorAll("script");
            scripts.forEach((oldScript) => {
                const newScript = document.createElement("script");
                Array.from(oldScript.attributes).forEach((attr) => {
                    newScript.setAttribute(attr.name, attr.value);
                });
                newScript.appendChild(document.createTextNode(oldScript.innerHTML));
                oldScript.parentNode?.replaceChild(newScript, oldScript);
            });
        }
    }, [old]);

    const view = useMemo(() => {
        if(!data) return null;
        // /api/batext is application/octet-stream, so useGetApi (real sync and
        // demo mock) hands us an ArrayBuffer.
        return new DataView(data);
    }, [data]);

    // Number of cells for the primary battery (needed by e.g. the BYD view's
    // "Detected cells" row). The main datalayer count is not part of the
    // batext blob.
    const cellsData = useGetApi('/api/cells', 5000);
    const cells = cellsData?.battery?.[0]?.voltages?.length;
    
    const btype = view?.getUint32(0, true) || 0;
    //const fields = FIELD_LISTS[btype] || [];

    // <Basic view={ view } fields={ fields } />

    // These commands all have one command per 'supports' flag
    const commands_list = [
        ["reset_soh", "Reset SoH"],
        ["reset_crash", "Reset crash"],
        ["clear_isolation", "Clear isolation fault"],
        ["reset_bms", "Reset BMS"],
        ["reset_soc", "Reset SoC"],
        ["reset_nvrol", "Reset NVROL"],
        ["reset_dtc", "Reset DTCs"],
        ["read_dtc", "Read DTCs"],
        ["reset_becm", "Restart BECM module"],
        ["calibrate_soc", "Calibrate SoC"],
        ["contactor_reset", "Contactor reset"],
        ["toggle_soc_method", "Toggle SoC method"],
        ["energy_saving_mode_reset", "Energy saving mode reset"],
        ["factory_mode_method", "Factory mode method"],
        ["chademo_restart", "Chademo restart"],
        ["chademo_stop", "Chademo stop"],
        ];

    return (
        <>
            <h2>Extended battery info</h2>

            { view && (BATTERY_EXT[btype] ? (() => {
                const BatteryView = BATTERY_EXT[btype];
                return <BatteryView view={ view } battery={ 1 } cells={ cells } />;
              })() : 
                <div ref={oldContainerRef} dangerouslySetInnerHTML={{__html: old }}></div>
            ) }

            <hr />

            { commands?.battery?.map((bat: any) => (
                <div key={ bat.id }>
                    <h3>Battery { bat.id }</h3>
                    { commands_list.map(([cmd, label]) => (
                        bat.commands?.[cmd] && <button key={ cmd } onClick={command(bat.id, cmd)}>{ label }</button>
                    )) }
                    { bat.commands?.balancing && <>
                        <button onClick={command(bat.id, "start_balancing")}>Start balancing</button>
                        { bat.commands?.balancing_active &&
                            <button onClick={command(bat.id, "stop_balancing")}>Stop balancing</button>
                        }
                    </> }
                    { bat.commands?.contactor_close && <>
                        <button onClick={command(bat.id, "contactor_close")}>Close contactors</button>
                        <button onClick={command(bat.id, "contactor_open")}>Open contactors</button>
                    </> }
                </div>
            )) }
        </>
    );
};
