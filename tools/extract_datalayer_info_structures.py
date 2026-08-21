#!/usr/bin/env python3
"""Extract DATALAYER_INFO_*/Battery struct layouts from DWARF.

Usage:
    extract_datalayer_info_structures.py [outDir] [objectFile ...]

- outDir: optional; when given, each structure is written to its own
  <slug>.ts file here instead of everything being concatenated to stdout.
- objectFile: optional list of .o/.elf files to process (DWARF is per
  translation unit). When omitted, falls back to the first
  .pio/build/*/firmware.elf (legacy).

DWARF DIE offsets are only unique within a single object file, so each file
is parsed in isolation and the resulting layouts are merged by name. A
struct/class is usually fully defined in several translation units (and
declared, without members, in the rest); the most complete definition wins.
"""

import glob
import os
import re
import subprocess
import sys

# Matches a DIE header line from `readelf -wi`, e.g.:
#   <2><2cadce>: Abbrev Number: 17 (DW_TAG_member)
#   <2><2cb166>: Abbrev Number: 0
# Attribute lines (indented, start with four spaces) do not match.
die_line_re = re.compile(
    r"^ <(\d+)><([0-9a-f]+)>: Abbrev Number: (\d+)(?: \((DW_TAG_\w+)\))?\s*$"
)

sizeof = {
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
}

type_names = {
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
}


def die_info(line):
    """Return (depth, offset, tag) for a DIE header line, or None."""
    m = die_line_re.match(line)
    if not m:
        return None
    return int(m.group(1)), m.group(2), m.group(4)


def attr_value(line):
    """Value after the last ': ', e.g. 'long unsigned int' from
    '    <2c5b68>   DW_AT_name : (indirect string, offset: 0x664e): long unsigned int'."""
    return line.split(": ")[-1].strip()


def parse_dwarf(lines):
    """Parse one `readelf -wi` dump into a {struct name -> layout} map."""
    types = {}
    sizes_by_id = {}
    typedef_targets = {}
    pointers = {}
    qualifiers = {}
    structs = {}

    def member_size(type_id, type_name):
        if type_name.endswith("*"):
            return 4  # ESP32 pointers are 4 bytes
        array_match = re.match(r"^(.+)\[(\d+)\]$", type_name)
        if array_match:
            return sizeof.get(array_match.group(1), 0) * int(array_match.group(2))
        if type_name in sizeof:
            return sizeof[type_name]
        # Typedefs point at the underlying struct/class DIE, which carries the byte size.
        target_id = typedef_targets.get(type_id) or type_id
        if target_id in sizes_by_id:
            return sizes_by_id[target_id]
        return 0

    # First pass: field types
    i = 0
    while i < len(lines):
        row = lines[i]
        if row.endswith(" (DW_TAG_typedef)") or row.endswith(" (DW_TAG_base_type)"):
            parts = row.split("<")
            if len(parts) > 2:
                type_id = parts[2].split(">")[0]
                while i + 1 < len(lines) and "DW_AT_name" not in lines[i]:
                    i += 1
                if i < len(lines) and "DW_AT_name" in lines[i]:
                    types[type_id] = attr_value(lines[i])
                if row.endswith(" (DW_TAG_typedef)"):
                    while i + 1 < len(lines) and die_info(lines[i + 1]) is None:
                        i += 1
                        type_match = re.search(r"DW_AT_type\s*:\s*<0x([0-9a-f]+)>", lines[i])
                        if type_match:
                            typedef_targets[type_id] = type_match.group(1)
        elif row.endswith(" (DW_TAG_array_type)"):
            parts = row.split("<")
            if len(parts) > 2:
                type_id = parts[2].split(">")[0]
                array_type_id = ""
                while i + 1 < len(lines) and not lines[i].endswith("(DW_TAG_subrange_type)"):
                    if "DW_AT_type" in lines[i]:
                        type_match = lines[i].split("<")
                        if len(type_match) > 2:
                            array_type_id = type_match[2].split(">")[0]
                    i += 1
                while i + 1 < len(lines) and not lines[i].endswith("Abbrev Number: 0"):
                    i += 1
                    if "DW_AT_upper_bound" in lines[i]:
                        bound_parts = lines[i].split(" : ")
                        upper_bound = bound_parts[1].split(" ")[0] if len(bound_parts) > 1 else ""
                        tid = array_type_id[2:] if array_type_id.startswith("0x") else array_type_id
                        if upper_bound.isdigit() and tid in types:
                            types[type_id] = "%s[%d]" % (types[tid], int(upper_bound) + 1)
        elif (
            row.endswith(" (DW_TAG_structure_type)")
            or row.endswith(" (DW_TAG_class_type)")
            or row.endswith(" (DW_TAG_union_type)")
            or row.endswith(" (DW_TAG_enumeration_type)")
        ):
            # Record the DIE id -> name so members referencing this type get a
            # readable name, and the byte size so member offsets can be tracked.
            parts = row.split("<")
            if len(parts) > 2:
                type_id = parts[2].split(">")[0]
                name = None
                size = None
                while i + 1 < len(lines) and die_info(lines[i + 1]) is None:
                    i += 1
                    line = lines[i]
                    if name is None and "DW_AT_name" in line:
                        name = attr_value(line)
                    if size is None and "DW_AT_byte_size" in line:
                        size_match = re.search(r"DW_AT_byte_size\s*:\s*(\d+)", line)
                        if size_match:
                            size = int(size_match.group(1))
                if name:
                    types[type_id] = name
                if size is not None:
                    sizes_by_id[type_id] = size
        elif row.endswith(" (DW_TAG_pointer_type)"):
            parts = row.split("<")
            if len(parts) > 2:
                type_id = parts[2].split(">")[0]
                name = None
                while i + 1 < len(lines) and die_info(lines[i + 1]) is None:
                    i += 1
                    line = lines[i]
                    if name is None and "DW_AT_name" in line:
                        name = attr_value(line)
                    type_match = re.search(r"DW_AT_type\s*:\s*<0x([0-9a-f]+)>", line)
                    if type_match:
                        pointers[type_id] = type_match.group(1)
                # Named pointer types (e.g. __vtbl_ptr_type) render as a readable,
                # build-independent name instead of a DIE offset.
                if name:
                    types[type_id] = name
        elif (
            row.endswith(" (DW_TAG_const_type)")
            or row.endswith(" (DW_TAG_volatile_type)")
            or row.endswith(" (DW_TAG_restrict_type)")
        ):
            # Qualifiers don't change size; just unwrap them so members keep a
            # readable, build-independent type name.
            parts = row.split("<")
            if len(parts) > 2:
                type_id = parts[2].split(">")[0]
                while i + 1 < len(lines) and die_info(lines[i + 1]) is None:
                    i += 1
                    type_match = re.search(r"DW_AT_type\s*:\s*<0x([0-9a-f]+)>", lines[i])
                    if type_match:
                        qualifiers[type_id] = type_match.group(1)
        i += 1

    # Resolve pointer types now that all names are known. Skip pointer DIEs
    # that already carry a name (e.g. __vtbl_ptr_type).
    for pointer_id, pointee_id in pointers.items():
        if pointer_id in types:
            continue
        types[pointer_id] = (types.get(pointee_id) or pointee_id) + "*"

    # Unwrap const/volatile qualifiers (they may nest and wrap pointers).
    for _ in range(8):
        changed = False
        for qual_id, pointee_id in qualifiers.items():
            resolved = types.get(pointee_id)
            if resolved and types.get(qual_id) != resolved:
                types[qual_id] = resolved
                changed = True
        if not changed:
            break

    # Second pass: structures and classes
    i = 0
    while i < len(lines):
        info = die_info(lines[i])
        if info and info[2] in ("DW_TAG_structure_type", "DW_TAG_class_type"):
            # Collect header attributes (name, byte_size, declaration) up to the
            # first child DIE. Skip forward declarations (DW_AT_declaration) -
            # they have no members and would otherwise shadow the real definition.
            header = {}
            j = i + 1
            while j < len(lines) and die_info(lines[j]) is None:
                attr_match = re.search(r"DW_AT_(\w+)", lines[j])
                if attr_match:
                    header[attr_match.group(1)] = lines[j]
                j += 1

            if "declaration" not in header:
                name = attr_value(header["name"]) if "name" in header else ""
                members = []
                bases = []
                byte_size = None
                size_match = re.search(r":\s*(\d+)\s*$", header.get("byte_size", ""))
                if size_match:
                    byte_size = int(size_match.group(1))

                # Walk the children. Only DW_TAG_member DIEs are data fields;
                # subprograms (methods), static consts, nested types and
                # inheritance are skipped.
                current_tag = None
                k = j
                while k < len(lines):
                    di = die_info(lines[k])
                    if di and di[0] <= info[0]:
                        break
                    row = lines[k]
                    if di:
                        current_tag = di[2]
                        k += 1
                        continue

                    if current_tag == "DW_TAG_member":
                        if "DW_AT_name" in row:
                            members.append([attr_value(row), None, None, 0])
                        if "DW_AT_data_member_location" in row:
                            loc_match = re.search(r"DW_AT_data_member_location:\s*(\d+)\s*$", row)
                            if loc_match and members:
                                members[-1][1] = int(loc_match.group(1))
                        if "DW_AT_type" in row:
                            type_match = row.split("<")
                            if len(type_match) > 2 and members:
                                type_id = type_match[2].split(">")[0]
                                if type_id.startswith("0x"):
                                    type_id = type_id[2:]
                                type_name = types.get(type_id) or type_id
                                entry = members[-1]
                                entry[2] = type_name
                                entry[3] = member_size(type_id, type_name)
                    elif current_tag == "DW_TAG_inheritance":
                        if "DW_AT_type" in row:
                            type_match = row.split("<")
                            if len(type_match) > 2:
                                type_id = type_match[2].split(">")[0]
                                if type_id.startswith("0x"):
                                    type_id = type_id[2:]
                                bases.append([types.get(type_id) or type_id, -1])
                        if "DW_AT_data_member_location" in row:
                            loc_match = re.search(r"DW_AT_data_member_location:\s*(\d+)\s*$", row)
                            if loc_match and bases:
                                bases[-1][1] = int(loc_match.group(1))
                    k += 1

                if name:
                    structs[name] = {"members": members, "bases": bases, "byte_size": byte_size}
                i = k - 1
            else:
                i = j - 1
        i += 1

    return structs


def layout_score(layout):
    score = 0
    if layout["byte_size"] is not None:
        score += 2
    if layout["bases"]:
        score += 1
    score += min(len(layout["members"]), 2)
    return score


def merge_structs(target, source):
    """Merge layouts by name, keeping the most complete definition."""
    for name, layout in source.items():
        existing = target.get(name)
        if not existing or layout_score(layout) > layout_score(existing):
            target[name] = layout


def collect_inherited_fields(structs, base_name, base_offset, depth):
    result = []
    base_layout = structs.get(base_name)
    if not base_layout:
        return result

    # Recurse into base's own bases first (deeper ancestors)
    for inner_base, inner_offset in base_layout["bases"]:
        result.extend(collect_inherited_fields(structs, inner_base, base_offset + inner_offset, depth + 1))

    # Then this base's own members
    prefix = "_" * depth
    for field, location, type_name, size in base_layout["members"]:
        if location is None or type_name is None:
            continue
        result.append([prefix + field, location + base_offset, type_name, size])

    return result


def emit_layout(structs, name, layout, is_class):
    out = ""
    upper = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", name).upper()
    if is_class:
        comment = "// %s" % name
        if layout["byte_size"] is not None:
            comment += ": %d bytes" % layout["byte_size"]
        if layout["bases"]:
            comment += "; base classes: " + ", ".join(
                "%s@%d" % (base, off) for base, off in layout["bases"]
            )
        out += comment + "\n"

    # Build a flat, offset-sorted list of all fields (inherited + own)
    all_members = []
    for base_name, base_offset in layout["bases"]:
        all_members.extend(collect_inherited_fields(structs, base_name, base_offset, 1))
    all_members.extend(layout["members"])
    all_members.sort(key=lambda m: m[1] if m[1] is not None else 0)

    out += "export const %s_FIELDS: ([string, string] | [string, string, number])[] = [\n" % upper
    offset = 0
    seen_count = {}
    for field, location, type_name, size in all_members:
        if location is None or type_name is None:
            continue

        gap = location - offset
        if gap > 0:
            if gap > 1:
                out += "  ['', ' ', %d],\n" % gap
            else:
                out += "  ['', ' '],\n"
            offset += gap

        if field == "battery_manufactureDate":
            continue

        deduped_field = field
        if field in seen_count:
            deduped_field = "%s_%d" % (field, seen_count[field] + 1)
        seen_count[field] = seen_count.get(field, 0) + 1

        array_match = re.match(r"^(.+)\[(\d+)\]$", type_name)
        if array_match:
            base_type = array_match.group(1)
            array_size = int(array_match.group(2))
            out += "  ['%s', '%s', %d],\n" % (deduped_field, type_names.get(base_type, base_type), array_size)
            offset += array_size * sizeof.get(base_type, 0)
        else:
            out += "  ['%s', '%s'],\n" % (deduped_field, type_names.get(type_name, type_name))
            offset += size
    out += "];\n\n"

    # Flat per-field consts: `export const <field>: [number, string] = [off, 'type']`.
    # Fresh dedup map: dedup runs per emission pass, so names match the old
    # object-map keys (first occurrence unsuffixed, repeats get _2, _3, ...).
    seen_count2 = {}
    for field, location, type_name, _size in all_members:
        if location is None or type_name is None:
            continue
        if field == "battery_manufactureDate":
            continue

        deduped_field = field
        if field in seen_count2:
            deduped_field = "%s_%d" % (field, seen_count2[field] + 1)
        seen_count2[field] = seen_count2.get(field, 0) + 1

        array_match = re.match(r"^(.+)\[(\d+)\]$", type_name)
        if array_match:
            out += "export const %s: [number, string, number] = [%d, '%s', %d];\n" % (
                deduped_field,
                location,
                type_names.get(array_match.group(1), array_match.group(1)),
                int(array_match.group(2)),
            )
        else:
            out += "export const %s: [number, string] = [%d, '%s'];\n" % (
                deduped_field,
                location,
                type_names.get(type_name, type_name),
            )
    out += "\n"
    return out


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else None
    file_args = sys.argv[2:]

    if file_args:
        object_files = file_args
    else:
        object_files = sorted(glob.glob(".pio/build/*/firmware.elf"))

    if not object_files:
        sys.exit(1)

    structs = {}
    for object_file in object_files:
        proc = subprocess.run(
            ["readelf", "-wi", object_file],
            capture_output=True,
            text=True,
            encoding="utf-8",
        )
        if proc.returncode != 0:
            print("readelf failed on " + object_file, file=sys.stderr)
            sys.exit(1)
        merge_structs(structs, parse_dwarf(proc.stdout.splitlines()))

    for name, layout in structs.items():
        if name.startswith("DATALAYER_INFO_"):
            is_class = False
        elif name.endswith("Battery"):
            is_class = True
        else:
            continue

        text = emit_layout(structs, name, layout, is_class)
        if out_dir:
            os.makedirs(out_dir, exist_ok=True)
            with open(os.path.join(out_dir, name + ".ts"), "w") as f:
                f.write(text.rstrip() + "\n")
        else:
            sys.stdout.write(text)

    if out_dir:
        # Remove generated files for structs that no longer exist, so stale
        # layouts don't linger in the frontend.
        generated = {
            name + ".ts"
            for name in structs
            if name.startswith("DATALAYER_INFO_") or name.endswith("Battery")
        }
        for f in os.listdir(out_dir):
            if f.endswith(".ts") and f not in generated:
                os.remove(os.path.join(out_dir, f))


if __name__ == "__main__":
    main()
