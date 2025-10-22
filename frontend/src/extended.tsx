import { useMemo } from "preact/hooks";

import { useGetApi } from "./utils/api.tsx";
import { DATALAYER_INFO_TESLA } from "./ext/datalayer.ts";

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
        var offset = 0;
        var rows: [string, number | any[]][] = [];
        DATALAYER_INFO_TESLA.forEach(([name, type, arr]: any[]) => {
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
