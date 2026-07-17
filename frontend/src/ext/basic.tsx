import { useMemo } from "preact/hooks";

import { FUNCS, LENGTHS } from "./consts.ts";

export function Basic({view, fields}: {view: DataView | null, fields: any[]}) {
    const rows = useMemo(() => {
        if(!view) return [];

        var offset = 4;
        var rows: [string, number | any[]][] = [];
        fields.forEach(([name, type, arr]: any[]) => {
            // Look up the length of this field
            const length = LENGTHS[type as keyof typeof LENGTHS];
            if(offset + (length * (arr ? arr : 1)) > view.byteLength) return;
            if(type == ' ') {
                // Is padding, skip over it
                offset += arr ? arr : length;
            } else if(arr) {
                // Is an array
                const varr = [];
                for(let i=0; i<arr; i++) {
                    const v = (view[FUNCS[type] as keyof DataView] as any)(offset, true);
                    offset += length;
                    varr.push(v);
                }
                rows.push([name, varr]);
            } else {
                const v = (view[FUNCS[type] as keyof DataView] as any)(offset, true);
                offset += LENGTHS[type as keyof typeof LENGTHS];
                rows.push([name, v]);
            }
        });
        return rows;
    }, [view, fields]);

    return <>
        { rows.map(([name, value]) => 
            <><strong>{ name }:</strong> { 
                value instanceof Array ?
                    value.join(', ') 
                    : value
            }<br /></>
        ) }
    </>;
}