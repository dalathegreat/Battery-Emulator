#!/usr/bin/env python3
"""Keep battery_detects_capacity() in sync with what the drivers actually do.

The settings page hides the "Battery capacity" row for integrations that
write datalayer.battery.info.total_capacity_Wh themselves, because on those
the stored value is overwritten and the Edit button never sticks. Which
integrations those are lives in one switch, battery_detects_capacity() in
Software/src/battery/BATTERIES.cpp.

A switch is exact and costs a few bytes of flash, but nothing stops a new
driver from assigning total_capacity_Wh and never being listed - the UI
would then offer an edit that silently reverts. This script closes that
gap: it derives the set from the sources and fails if the switch disagrees.

How the set is derived
  1. Every battery source that assigns `info.total_capacity_Wh` is found,
     ignoring comments and comparisons. The enclosing `Class::method` tells
     us which driver class does the writing.
  2. A class inherits the property from its base (MqbEvoBattery gets it from
     MebBattery, whose update_values() it reuses).
  3. create_battery() in BATTERIES.cpp maps each class back to the
     BatteryType values that instantiate it - several types can share one
     class, as Tesla Model 3/Y and Model S/X do.
  4. The result is compared against the case labels of
     battery_detects_capacity().

Exit code is non-zero on any disagreement, in either direction: a driver
that writes the field but is missing from the switch, or a type listed in
the switch whose driver no longer writes it.
"""

import argparse
import re
import sys
from pathlib import Path

# An assignment, not a comparison: reject ==, !=, >=, <=.
ASSIGNMENT = re.compile(r"\binfo\.total_capacity_Wh\s*(?<![=!<>])=(?!=)")

# A definition, not a qualified call. Definitions start in column 0, so the
# leading ^[A-Za-z_] rules out `  MebBattery::update_values();` inside a body.
# clang-format happily wraps a long signature after the `::`, as it does for
# `void MebBattery::\n    update_values() {`, so \s* has to span newlines.
METHOD_DEF = re.compile(r"^[A-Za-z_][^\n;(){}=]*?\b(\w+)\s*::\s*(\w+)\s*\(", re.M)
INHERITANCE = re.compile(r"\bclass\s+(\w+)\s*:\s*(?:public|protected|private)\s+(\w+)")
CASE_LABEL = re.compile(r"case\s+BatteryType::(\w+)\s*:")
RETURN_NEW = re.compile(r"return\s+new\s+(\w+)\s*\(")


def strip_comments(text):
    """Blank out comments, keeping line numbering intact.

    This has to be a scanner rather than two regex passes. The drivers are full
    of lines like `battery_SOC = x * 5;  //*0.05*100`, where a naive /* */ pass
    sees the `/*` inside the line comment and swallows everything up to the next
    `*/` hundreds of lines later - assignments included. Strings are tracked for
    the same reason: a `//` inside one must not end the line.
    """
    out, i, n = [], 0, len(text)
    while i < n:
        two = text[i : i + 2]
        if two == "//":
            while i < n and text[i] != "\n":
                out.append(" ")
                i += 1
        elif two == "/*":
            while i < n and text[i : i + 2] != "*/":
                out.append("\n" if text[i] == "\n" else " ")
                i += 1
            out.append("  ")
            i += 2
        elif text[i] in "\"'":
            quote = text[i]
            out.append(quote)
            i += 1
            while i < n and text[i] != quote:
                if text[i] == "\\" and i + 1 < n:
                    out.append("  ")
                    i += 2
                    continue
                out.append("\n" if text[i] == "\n" else " ")
                i += 1
            if i < n:
                out.append(quote)
                i += 1
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


def extract_block(text, signature):
    """Return the body of a function, from its signature to the closing brace."""
    start = text.find(signature)
    if start < 0:
        return None
    depth, i = 0, text.index("{", start)
    for pos in range(i, len(text)):
        if text[pos] == "{":
            depth += 1
        elif text[pos] == "}":
            depth -= 1
            if depth == 0:
                return text[i : pos + 1]
    return None


def classes_writing_capacity(battery_dir):
    """Driver classes with at least one `info.total_capacity_Wh =` in them."""
    writers, unattributed = {}, []
    for path in sorted(battery_dir.glob("*.cpp")) + sorted(battery_dir.glob("*.h")):
        text = strip_comments(path.read_text(errors="replace"))
        definitions = [(m.start(), m.group(1)) for m in METHOD_DEF.finditer(text)]
        for assignment in ASSIGNMENT.finditer(text):
            enclosing = [cls for start, cls in definitions if start < assignment.start()]
            where = f"{path.name}:{text.count(chr(10), 0, assignment.start()) + 1}"
            if not enclosing:
                unattributed.append(where)
                continue
            writers.setdefault(enclosing[-1], []).append(where)
    return writers, unattributed


def inheritance_map(src_dir):
    return dict(INHERITANCE.findall(strip_comments(
        "\n".join(p.read_text(errors="replace") for p in sorted(src_dir.rglob("*.h"))))))


def inherits_from_writer(cls, bases, writers):
    """Walk up the base chain; a derived class reuses its base's writes."""
    seen = set()
    while cls in bases and cls not in seen:
        seen.add(cls)
        cls = bases[cls]
        if cls in writers:
            return cls
    return None


def parse_switch(body, want_class_map):
    """Read `case BatteryType::X:` labels, optionally with their `return new C()`."""
    result, pending = {}, []
    for line in body.split("\n"):
        for label in CASE_LABEL.findall(line):
            pending.append(label)
        target = RETURN_NEW.search(line) if want_class_map else re.search(r"return\s+true\s*;", line)
        if target and pending:
            for label in pending:
                result[label] = target.group(1) if want_class_map else True
            pending = []
        elif re.search(r"return\s+(false|nullptr)\s*;", line):
            pending = []
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--root", default=Path(__file__).resolve().parent.parent, type=Path,
                        help="repository root (default: the checkout this script lives in)")
    args = parser.parse_args()

    battery_dir = args.root / "Software" / "src" / "battery"
    batteries_cpp = (battery_dir / "BATTERIES.cpp").read_text(errors="replace")
    source = strip_comments(batteries_cpp)

    factory = extract_block(source, "Battery* create_battery(")
    predicate = extract_block(source, "bool battery_detects_capacity(")
    if factory is None or predicate is None:
        sys.exit("could not locate create_battery() or battery_detects_capacity() in BATTERIES.cpp")

    writers, unattributed = classes_writing_capacity(battery_dir)
    bases = inheritance_map(args.root / "Software" / "src")
    type_to_class = parse_switch(factory, want_class_map=True)
    listed = set(parse_switch(predicate, want_class_map=False))

    expected, why = set(), {}
    for battery_type, cls in type_to_class.items():
        if cls in writers:
            expected.add(battery_type)
            why[battery_type] = writers[cls][0]
        else:
            base = inherits_from_writer(cls, bases, writers)
            if base:
                expected.add(battery_type)
                why[battery_type] = f"{writers[base][0]} (inherited from {base})"

    missing = sorted(expected - listed)
    stale = sorted(listed - expected)

    for battery_type in missing:
        print(f"MISSING: BatteryType::{battery_type} writes total_capacity_Wh at {why[battery_type]}")
        print("         but is not listed in battery_detects_capacity(), so the settings page")
        print("         still offers a Battery capacity edit that the driver will overwrite.")
    for battery_type in stale:
        print(f"STALE:   BatteryType::{battery_type} is listed in battery_detects_capacity(),")
        print("         but its driver no longer assigns total_capacity_Wh. The settings page")
        print("         hides a row the user now needs.")
    for where in unattributed:
        print(f"UNKNOWN: {where} assigns total_capacity_Wh outside any Class::method this script")
        print("         recognises, so it cannot be attributed to a BatteryType. Check the")
        print("         METHOD_DEF pattern here against however that code is written.")

    if missing or stale or unattributed:
        print(f"\n{len(missing) + len(stale) + len(unattributed)} problem(s). Update "
              "battery_detects_capacity() in Software/src/battery/BATTERIES.cpp.")
        return 1

    print(f"battery_detects_capacity() matches the drivers: {len(expected)} integrations detect their capacity.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
