export const DATALAYER_INFO_BMWIX_FIELDS: ([string, string] | [string, string, number])[] = [
  ['dtc_codes', 'u32', 32],
  ['dtc_status', 'u8', 32],
  ['dtc_last_read_millis', 'u32'],
  ['dtc_count', 'u8'],
  ['dtc_read_in_progress', 'b'],
  ['dtc_read_failed', 'b'],
  ['UserRequestDTCreset', 'b'],
  ['UserRequestBMSReset', 'b'],
];

export const dtc_codes: [number, string, number] = [0, 'u32', 32];
export const dtc_status: [number, string, number] = [128, 'u8', 32];
export const dtc_last_read_millis: [number, string] = [160, 'u32'];
export const dtc_count: [number, string] = [164, 'u8'];
export const dtc_read_in_progress: [number, string] = [165, 'b'];
export const dtc_read_failed: [number, string] = [166, 'b'];
export const UserRequestDTCreset: [number, string] = [167, 'b'];
export const UserRequestBMSReset: [number, string] = [168, 'b'];
