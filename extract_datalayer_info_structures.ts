import { spawnSync } from "bun";
import { Glob } from "bun";

const glob = new Glob(".pio/build/*/firmware.elf");
const elfFiles = Array.from(glob.scanSync({ dot: true }));

if (elfFiles.length === 0) {
  process.exit(1);
}

const elfFile = elfFiles[0];

const readelf = Bun.spawnSync(["readelf", "-wi", elfFile]);
const output = new TextDecoder().decode(readelf.stdout);
const lines = output.split(/\r?\n/);

const types: Record<string, string> = {};
const structs: Record<string, [string, number | null, string | null][]> = {};

// First pass: field types
let i = 0;
while (i < lines.length) {
  let row = lines[i];
  if (row.endsWith(" (DW_TAG_typedef)") || row.endsWith(" (DW_TAG_base_type)")) {
    const parts = row.split("<");
    if (parts.length > 2) {
      const id = parts[2].split(">")[0];
      while (i + 1 < lines.length && !lines[i].includes("DW_AT_name")) {
        i++;
      }
      if (i < lines.length && lines[i].includes("DW_AT_name")) {
        const name = lines[i].trim().split(/\s+/).pop()!;
        types[id] = name;
      }
    }
  } else if (row.endsWith(" (DW_TAG_array_type)")) {
    const parts = row.split("<");
    if (parts.length > 2) {
      const id = parts[2].split(">")[0];
      let type_id = "";
      while (i + 1 < lines.length && !lines[i].endsWith("(DW_TAG_subrange_type)")) {
        if (lines[i].includes("DW_AT_type")) {
          const typeMatch = lines[i].split("<")[2]?.split(">")[0];
          if (typeMatch) type_id = typeMatch;
        }
        i++;
      }
      while (i + 1 < lines.length && !lines[i].endsWith("Abbrev Number: 0")) {
        i++;
        if (lines[i].includes("DW_AT_upper_bound")) {
          const boundParts = lines[i].split(" : ");
          const upper_bound = boundParts[1]?.split(" ")[0];
          if (upper_bound && /^\d+$/.test(upper_bound) && types[type_id.startsWith("0x") ? type_id.slice(2) : type_id]) {
            const tid = type_id.startsWith("0x") ? type_id.slice(2) : type_id;
            types[id] = `${types[tid]}[${parseInt(upper_bound) + 1}]`;
          }
        }
      }
    }
  }
  i++;
}

// Second pass: structures
i = 0;
while (i < lines.length) {
  let row = lines[i];
  if (row.endsWith(" (DW_TAG_structure_type)")) {
    i++;
    if (i < lines.length) {
      const name = lines[i].trim().split(/\s+/).pop()!;
      const struct: [string, number | null, string | null][] = [];

      while (i + 1 < lines.length && !lines[i].endsWith("Abbrev Number: 0")) {
        i++;
        row = lines[i];
        if (row.includes("DW_AT_name")) {
          const field = row.trim().split(/\s+/).pop()!;
          struct.push([field, null, null]);
        }

        if (row.includes("DW_AT_data_member_location")) {
          const location = parseInt(row.trim().split(/\s+/).pop()!);
          if (struct.length > 0) {
            struct[struct.length - 1][1] = location;
          }
        }

        if (row.includes("DW_AT_type")) {
          const typeMatch = row.split("<")[2]?.split(">")[0];
          if (typeMatch) {
            const type_id = typeMatch.startsWith("0x") ? typeMatch.slice(2) : typeMatch;
            const type_name = types[type_id] || type_id;
            if (struct.length > 0) {
              struct[struct.length - 1][2] = type_name;
            }
          }
        }
      }
      structs[name] = struct;
    }
  }
  i++;
}

const sizeof: Record<string, number> = {
  "uint8_t": 1,
  "uint16_t": 2,
  "uint32_t": 4,
  "uint64_t": 8,
  "int8_t": 1,
  "int16_t": 2,
  "int32_t": 4,
  "int": 4,
  "char": 1,
  "float": 4,
  "double": 8,
  "bool": 1,
};

const type_names: Record<string, string> = {
  "uint8_t": "u8",
  "uint16_t": "u16",
  "uint32_t": "u32",
  "uint64_t": "u64",
  "int8_t": "i8",
  "int16_t": "i16",
  "int32_t": "i32",
  "int": "i32",
  "float": "f",
  "bool": "b",
};

for (const [name, struct] of Object.entries(structs)) {
  if (!name.startsWith("DATALAYER_INFO_")) continue;

  console.log(`export const ${name}_FIELDS: ([string, string] | [string, string, number])[] = [`);
  let offset = 0;
  for (const [field, location, type_name] of struct) {
    if (location === null || type_name === null) continue;

    const gap = location - offset;
    if (gap > 0) {
      if (gap > 1) {
        console.log(`  ['', ' ', ${gap}],`);
      } else {
        console.log(`  ['', ' '],`);
      }
      offset += gap;
    }

    if (field === "battery_manufactureDate") continue;

    if (type_name.includes("[")) {
      const [base_type, arrayPart] = type_name.split("[");
      const array_size = parseInt(arrayPart.replace("]", ""));
      console.log(`  ['${field}', '${type_names[base_type] || base_type}', ${array_size}],`);
      offset += array_size * (sizeof[base_type] || 0);
    } else {
      console.log(`  ['${field}', '${type_names[type_name] || type_name}'],`);
      offset += sizeof[type_name] || 0;
    }
  }
  console.log("];\n");

  console.log(`export const ${name}: { [key: string]: [number, string] | [number, string, number] } = {`);
  for (const [field, location, type_name] of struct) {
    if (location === null || type_name === null) continue;
    if (field === "battery_manufactureDate") continue;

    if (type_name.includes("[")) {
      const [base_type, arrayPart] = type_name.split("[");
      const array_size = parseInt(arrayPart.replace("]", ""));
      console.log(`  ${field}: [${location}, '${type_names[base_type] || base_type}', ${array_size}],`);
    } else {
      console.log(`  ${field}: [${location}, '${type_names[type_name] || type_name}'],`);
    }
  }
  console.log("};\n");
}
