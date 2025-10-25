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
    'b': 1,
    ' ': 1,
};