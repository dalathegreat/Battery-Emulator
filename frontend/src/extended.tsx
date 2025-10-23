import { useMemo } from "preact/hooks";

import { useGetApi } from "./utils/api.tsx";
import { 
    DATALAYER_INFO_BOLTAMPERA,
    DATALAYER_INFO_BMWPHEV,
    DATALAYER_INFO_BYDATTO3,
    DATALAYER_INFO_CELLPOWER,
    DATALAYER_INFO_CHADEMO,
    DATALAYER_INFO_CMFAEV,
    DATALAYER_INFO_ECMP,
    DATALAYER_INFO_GEELY_GEOMETRY_C,
    DATALAYER_INFO_KIAHYUNDAI64,
    DATALAYER_INFO_TESLA,
    DATALAYER_INFO_NISSAN_LEAF,
    DATALAYER_INFO_MEB,
    DATALAYER_INFO_VOLVO_POLESTAR,
    DATALAYER_INFO_VOLVO_HYBRID,
    DATALAYER_INFO_ZOE,
    DATALAYER_INFO_ZOE_PH2
} from "./ext/datalayer.ts";

export function Extended() {
    const data = useGetApi('/api/batext', 5000);

    const funcs: { [key: string]: string } = {
        'u8': 'getUint8',
        'u16': 'getUint16',
        'u32': 'getUint32',
        'u64': 'getBigUint64',
        'i8': 'getInt8',
        'i16': 'getInt16',
        'i32': 'getInt32',
        'f': 'getFloat32',
        'b': 'getUint8',
    };
    const lengths: { [key: string]: number } = {
        'u8': 1,
        'u16': 2,
        'u32': 4,
        'u64': 8,
        'i8': 1,
        'i16': 2,
        'i32': 4,
        'b': 1,
        ' ': 1,
    };

    const rows = useMemo(() => {
        if(!data) return [];
        const buf = new Uint8Array(data.length);
        for(let i=0; i<data.length; i++) {
            buf[i] = data.charCodeAt(i);
        }
        const view = new DataView(buf.buffer);
        const btype = view.getUint32(0, true);

        const specs = {
            4: DATALAYER_INFO_BOLTAMPERA,
            43: DATALAYER_INFO_BMWPHEV,
            5: DATALAYER_INFO_BYDATTO3,
            6: DATALAYER_INFO_CELLPOWER,
            7: DATALAYER_INFO_CHADEMO,
            8: DATALAYER_INFO_CMFAEV,
            13: DATALAYER_INFO_ECMP,
            10: DATALAYER_INFO_GEELY_GEOMETRY_C,
            17: DATALAYER_INFO_KIAHYUNDAI64,
            32: DATALAYER_INFO_TESLA,
            33: DATALAYER_INFO_TESLA,
            21: DATALAYER_INFO_NISSAN_LEAF,
            19: DATALAYER_INFO_MEB,
            35: DATALAYER_INFO_VOLVO_POLESTAR,
            36: DATALAYER_INFO_VOLVO_HYBRID,
            28: DATALAYER_INFO_ZOE,
            29: DATALAYER_INFO_ZOE_PH2,
        } as { [key: number]: any[] };

        var offset = 4;
        var rows: [string, number | any[]][] = [];
        specs[btype].forEach(([name, type, arr]: any[]) => {
            const length = lengths[type as keyof typeof lengths];
            if(type == ' ') {
                offset += arr ? arr : length;
            } else if(arr) {
                const varr = [];
                for(let i=0; i<arr; i++) {
                    const v = view[funcs[type] as keyof DataView](offset, true);
                    offset += length;
                    varr.push(v);
                }
                rows.push([name, varr]);
            } else {
                const v = view[funcs[type] as keyof DataView](offset, true);
                offset += lengths[type as keyof typeof lengths];
                rows.push([name, v]);
            }
        });
        return rows;
    }, [data]);

    return (
        <>
            <h2>Extended Features</h2>
            
            { rows.map(([name, value]) => 
                <><strong>{ name }:</strong> { value }<br /></>
            ) }
            
        </>
    );
};
