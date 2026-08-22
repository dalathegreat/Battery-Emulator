export const FUNCS: { [key: string]: string } = {
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

export const LENGTHS: { [key: string]: number } = {
    'u8': 1,
    'u16': 2,
    'u32': 4,
    'u64': 8,
    'i8': 1,
    'i16': 2,
    'i32': 4,
    'f': 4,
    'b': 1,
    ' ': 1,
};

// A single datalayer field: [offset, type] or [offset, type, count] (count = array length).
export type Field = [number, string] | [number, string, number];

// The /api/batext payload is [u32 battery-type header][struct bytes], so the
// field offsets in the datalayer tables (relative to the struct) are shifted
// by this 4-byte header.
export const BATEXT_HEADER = 4;

// Read a datalayer field. Curried so callers bind the DataView once and then
// read fields by offset: `const get = lookup(view); get(L.someField)`.
// Returns a single value, or an array when the field is a fixed-size array.
// The bytes are untyped/raw, so callers cast to the expected type.
export function lookup(view: DataView) {
    return (field: Field): unknown => {
        const [offset, type, count] = field;
        // Index a DataView method by name from FUNCS; the union of getter types
        // isn't callable directly, so widen to the common signature.
        const read = (view[FUNCS[type] as keyof DataView] as unknown as (off: number, littleEndian: boolean) => unknown).bind(view);
        if(count !== undefined) {
            const arr: unknown[] = [];
            for(let i=0; i<count; i++) {
                arr.push(read(BATEXT_HEADER + offset + i * LENGTHS[type], true));
            }
            return arr;
        }
        return read(BATEXT_HEADER + offset, true);
    };
}