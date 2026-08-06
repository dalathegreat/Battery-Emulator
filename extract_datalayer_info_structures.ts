import { spawnSync } from "bun";
import { Glob } from "bun";
import { mkdirSync, writeFileSync } from "node:fs";
import { join } from "node:path";

// Optional output directory: when given, each structure is written to its own
// <slug>.ts file here instead of everything being concatenated to stdout.
const outDir = process.argv[2];

const glob = new Glob(".pio/build/*/firmware.elf");
const elfFiles = Array.from(glob.scanSync({ dot: true }));

if (elfFiles.length === 0) {
  process.exit(1);
}

const elfFile = elfFiles[0];

const readelf = Bun.spawnSync(["readelf", "-wi", elfFile]);
const output = new TextDecoder().decode(readelf.stdout);
const lines = output.split(/\r?\n/);

// Matches a DIE header line from `readelf -wi`, e.g.:
//   <2><2cadce>: Abbrev Number: 17 (DW_TAG_member)
//   <2><2cb166>: Abbrev Number: 0
// Attribute lines (indented, start with four spaces) do not match.
const dieLineRe = /^ <(\d+)><([0-9a-f]+)>: Abbrev Number: (\d+)(?: \((DW_TAG_\w+)\))?\s*$/;

function dieInfo(line: string): { depth: number; offset: string; tag?: string } | null {
  const m = line.match(dieLineRe);
  if (!m) return null;
  return { depth: parseInt(m[1]), offset: m[2], tag: m[4] };
}

// Extract the value from a DW_AT_name attribute line, e.g.:
//   "    <2c5b68>   DW_AT_name : (indirect string, offset: 0x664e): long unsigned int"
//   -> "long unsigned int"
function attrValue(line: string): string {
  return line.split(": ").pop()!.trim();
}

const types: Record<string, string> = {};
const sizesById: Record<string, number> = {};
const typedefTargets: Record<string, string> = {};
const pointers: Record<string, string> = {};

interface Layout {
  members: [string, number | null, string | null, number][];
  bases: [string, number][];
  byteSize: number | null;
}
const structs: Record<string, Layout> = {};

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
  "unsigned char": 1,
  "signed char": 1,
  "short": 2,
  "unsigned short": 2,
  "unsigned int": 4,
  "long int": 4,
  "long unsigned int": 4,
  "long long int": 8,
  "long long unsigned int": 8,
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
  "unsigned char": "u8",
  "signed char": "i8",
  "short": "i16",
  "unsigned short": "u16",
  "unsigned int": "u32",
  "long int": "i32",
  "long unsigned int": "u32",
  "long long int": "i64",
  "long long unsigned int": "u64",
};

function memberSize(typeId: string, typeName: string): number {
  if (typeName.endsWith("*")) return 4; // ESP32 pointers are 4 bytes
  const arrayMatch = typeName.match(/^(.+)\[(\d+)\]$/);
  if (arrayMatch) {
    return (sizeof[arrayMatch[1]] || 0) * parseInt(arrayMatch[2]);
  }
  if (sizeof[typeName]) return sizeof[typeName];
  // Typedefs point at the underlying struct/class DIE, which carries the byte size.
  const targetId = typedefTargets[typeId] || typeId;
  if (sizesById[targetId]) return sizesById[targetId];
  return 0;
}

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
        types[id] = attrValue(lines[i]);
      }
      if (row.endsWith(" (DW_TAG_typedef)")) {
        while (i + 1 < lines.length && !dieInfo(lines[i + 1])) {
          i++;
          const typeMatch = lines[i].match(/DW_AT_type\s*:\s*<0x([0-9a-f]+)>/);
          if (typeMatch) typedefTargets[id] = typeMatch[1];
        }
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
  } else if (
    row.endsWith(" (DW_TAG_structure_type)") ||
    row.endsWith(" (DW_TAG_class_type)") ||
    row.endsWith(" (DW_TAG_union_type)") ||
    row.endsWith(" (DW_TAG_enumeration_type)")
  ) {
    // Record the DIE id -> name so members referencing this type get a readable
    // name, and the byte size so member offsets can be tracked.
    const parts = row.split("<");
    if (parts.length > 2) {
      const id = parts[2].split(">")[0];
      let name: string | null = null;
      let size: number | null = null;
      while (i + 1 < lines.length && !dieInfo(lines[i + 1])) {
        i++;
        const l = lines[i];
        if (name === null && l.includes("DW_AT_name")) {
          name = attrValue(l);
        }
        if (size === null && l.includes("DW_AT_byte_size")) {
          const sizeMatch = l.match(/DW_AT_byte_size\s*:\s*(\d+)/);
          if (sizeMatch) size = parseInt(sizeMatch[1]);
        }
      }
      if (name) types[id] = name;
      if (size !== null) sizesById[id] = size;
    }
  } else if (row.endsWith(" (DW_TAG_pointer_type)")) {
    const parts = row.split("<");
    if (parts.length > 2) {
      const id = parts[2].split(">")[0];
      while (i + 1 < lines.length && !dieInfo(lines[i + 1])) {
        i++;
        const typeMatch = lines[i].match(/DW_AT_type\s*:\s*<0x([0-9a-f]+)>/);
        if (typeMatch) pointers[id] = typeMatch[1];
      }
    }
  }
  i++;
}

// Resolve pointer types now that all names are known.
for (const [pointerId, pointeeId] of Object.entries(pointers)) {
  types[pointerId] = (types[pointeeId] || pointeeId) + "*";
}

// Second pass: structures and classes
i = 0;
while (i < lines.length) {
  const info = dieInfo(lines[i]);
  if (info && (info.tag === "DW_TAG_structure_type" || info.tag === "DW_TAG_class_type")) {
    // Collect header attributes (name, byte_size, declaration) up to the first
    // child DIE. Skip forward declarations (DW_AT_declaration) - they have no
    // members and would otherwise shadow the real definition.
    const header: Record<string, string> = {};
    let j = i + 1;
    while (j < lines.length && !dieInfo(lines[j])) {
      const attrMatch = lines[j].match(/DW_AT_(\w+)/);
      if (attrMatch) header[attrMatch[1]] = lines[j];
      j++;
    }

    if (!header.declaration) {
      const name = header.name ? attrValue(header.name) : "";
      const members: [string, number | null, string | null, number][] = [];
      const bases: [string, number][] = [];
      let byteSize: number | null = null;
      const sizeMatch = header.byte_size?.match(/:\s*(\d+)\s*$/);
      if (sizeMatch) byteSize = parseInt(sizeMatch[1]);

      // Walk the children. Only DW_TAG_member DIEs are data fields; subprograms
      // (methods), static consts, nested types and inheritance are skipped.
      let currentTag: string | null = null;
      let k = j;
      while (k < lines.length) {
        const di = dieInfo(lines[k]);
        if (di && di.depth <= info.depth) break;
        const row = lines[k];
        if (di) {
          currentTag = di.tag ?? null;
          k++;
          continue;
        }

        if (currentTag === "DW_TAG_member") {
          if (row.includes("DW_AT_name")) {
            const field = attrValue(row);
            members.push([field, null, null, 0]);
          }
          if (row.includes("DW_AT_data_member_location")) {
            const locMatch = row.match(/DW_AT_data_member_location:\s*(\d+)\s*$/);
            if (locMatch && members.length > 0) {
              members[members.length - 1][1] = parseInt(locMatch[1]);
            }
          }
          if (row.includes("DW_AT_type")) {
            const typeMatch = row.split("<")[2]?.split(">")[0];
            if (typeMatch && members.length > 0) {
              const type_id = typeMatch.startsWith("0x") ? typeMatch.slice(2) : typeMatch;
              const type_name = types[type_id] || type_id;
              const entry = members[members.length - 1];
              entry[2] = type_name;
              entry[3] = memberSize(type_id, type_name);
            }
          }
        } else if (currentTag === "DW_TAG_inheritance") {
          if (row.includes("DW_AT_type")) {
            const typeMatch = row.split("<")[2]?.split(">")[0];
            if (typeMatch) {
              const type_id = typeMatch.startsWith("0x") ? typeMatch.slice(2) : typeMatch;
              bases.push([types[type_id] || type_id, -1]);
            }
          }
          if (row.includes("DW_AT_data_member_location")) {
            const locMatch = row.match(/DW_AT_data_member_location:\s*(\d+)\s*$/);
            if (locMatch && bases.length > 0) {
              bases[bases.length - 1][1] = parseInt(locMatch[1]);
            }
          }
        }
        k++;
      }

      if (name) structs[name] = { members, bases, byteSize };
      i = k - 1;
    } else {
      i = j - 1;
    }
  }
  i++;
}

function collectInheritedFields(
  baseName: string,
  baseOffset: number,
  depth: number
): [string, number, string | null, number][] {
  const result: [string, number, string | null, number][] = [];
  const baseLayout = structs[baseName];
  if (!baseLayout) return result;

  // Recurse into base's own bases first (deeper ancestors)
  for (const [innerBase, innerOffset] of baseLayout.bases) {
    result.push(...collectInheritedFields(innerBase, baseOffset + innerOffset, depth + 1));
  }

  // Then this base's own members
  const prefix = "_".repeat(depth);
  for (const [field, location, typeName, size] of baseLayout.members) {
    if (location === null || typeName === null) continue;
    result.push([`${prefix}${field}`, location + baseOffset, typeName, size]);
  }

  return result;
}

function emitLayout(name: string, layout: Layout, isClass: boolean): string {
  let out = "";
  const upper = name.replace(/([a-z0-9])([A-Z])/g, "$1_$2").toUpperCase();
  if (isClass) {
    let comment = `// ${name}`;
    if (layout.byteSize !== null) comment += `: ${layout.byteSize} bytes`;
    if (layout.bases.length > 0) {
      comment += "; base classes: " + layout.bases.map(([base, off]) => `${base}@${off}`).join(", ");
    }
    out += comment + "\n";
  }

  // Build a flat, offset-sorted list of all fields (inherited + own)
  const allMembers: [string, number, string | null, number][] = [];
  for (const [baseName, baseOffset] of layout.bases) {
    allMembers.push(...collectInheritedFields(baseName, baseOffset, 1));
  }
  for (const m of layout.members) {
    allMembers.push(m);
  }
  allMembers.sort((a, b) => (a[1] ?? 0) - (b[1] ?? 0));

  out += `export const ${upper}_FIELDS: ([string, string] | [string, string, number])[] = [\n`;
  let offset = 0;
  const seenCount = new Map<string, number>();
  for (const [field, location, type_name, size] of allMembers) {
    if (location === null || type_name === null) continue;

    const gap = location - offset;
    if (gap > 0) {
      if (gap > 1) {
        out += `  ['', ' ', ${gap}],\n`;
      } else {
        out += `  ['', ' '],\n`;
      }
      offset += gap;
    }

    if (field === "battery_manufactureDate") continue;

    const dedupedField = seenCount.has(field) ? `${field}_${(seenCount.get(field)||0) + 1}` : field;
    seenCount.set(field, (seenCount.get(field) || 0) + 1);

    const arrayMatch = type_name.match(/^(.+)\[(\d+)\]$/);
    if (arrayMatch) {
      const base_type = arrayMatch[1];
      const array_size = parseInt(arrayMatch[2]);
      out += `  ['${dedupedField}', '${type_names[base_type] || base_type}', ${array_size}],\n`;
      offset += array_size * (sizeof[base_type] || 0);
    } else {
      out += `  ['${dedupedField}', '${type_names[type_name] || type_name}'],\n`;
      offset += size;
    }
  }
  out += "];\n\n";

  // Flat per-field consts: `export const <field>: [number, string] = [off, 'type']`.
  // Fresh dedup map: dedup runs per emission pass, so names match the old
  // object-map keys (first occurrence unsuffixed, repeats get _2, _3, ...).
  const seenCount2 = new Map<string, number>();
  for (const [field, location, type_name] of allMembers) {
    if (location === null || type_name === null) continue;
    if (field === "battery_manufactureDate") continue;

    const dedupedField = seenCount2.has(field) ? `${field}_${(seenCount2.get(field)||0) + 1}` : field;
    seenCount2.set(field, (seenCount2.get(field) || 0) + 1);

    const arrayMatch = type_name.match(/^(.+)\[(\d+)\]$/);
    if (arrayMatch) {
      out += `export const ${dedupedField}: [number, string, number] = [${location}, '${type_names[arrayMatch[1]] || arrayMatch[1]}', ${arrayMatch[2]}];\n`;
    } else {
      out += `export const ${dedupedField}: [number, string] = [${location}, '${type_names[type_name] || type_name}'];\n`;
    }
  }
  out += "\n";
  return out;
}

for (const [name, layout] of Object.entries(structs)) {
  let isClass: boolean;
  if (name.startsWith("DATALAYER_INFO_")) {
    isClass = false;
  } else if (name.endsWith("Battery")) {
    isClass = true;
  } else {
    continue;
  }

  const text = emitLayout(name, layout, isClass);
  if (outDir) {
    mkdirSync(outDir, { recursive: true });
    writeFileSync(join(outDir, name + ".ts"), text.trimEnd() + "\n");
  } else {
    process.stdout.write(text);
  }
}
