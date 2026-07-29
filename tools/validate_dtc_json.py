#!/usr/bin/env python3
"""Validate DTC description JSON files used by the web UI's DTC loader.

Each file must be a JSON array of objects. The in-page loader
(BatteryHtmlRenderer.h) builds a lookup like:

    arr.forEach(function(e){ if(e.code!=null) m[e.code]=e; if(e.dtc) m[e.dtc]=e; });

and fills any cell carrying data-dtc-code='<code-or-dtc>' with `l_dsc`
(plus `s_dsc` in <em> when non-empty). This script checks that every entry
is actually usable by that loader.

Rules
  ERROR (entry cannot work / file is malformed):
    - file is not valid JSON, or the root is not an array
    - an entry is not an object
    - `l_dsc` is missing, null, or empty  -> nothing to display
    - no usable key: `code` is null/missing AND `dtc` is missing/empty
                     -> entry can never be matched
    - `code` is an empty string ""        -> JS "" != null, pollutes m[""]
    - `code` is present but not an integer (string/float/bool)
    - `dtc`  is present but not a string

  WARNING (loads, but probably not intended):
    - `code` == 0                         -> creates m["0"]
    - duplicate `code` values             -> later entry overwrites earlier
    - duplicate `dtc` among entries that have NO `code`
                                          -> the string is the only key, last wins
    - unknown keys (outside code/dtc/s_dsc/l_dsc)
    - `s_dsc` present but not a string

Exit code is non-zero if any file has errors (or, with --strict, warnings).
"""

import argparse
import json
import sys
from collections import defaultdict

KNOWN_KEYS = {"code", "dtc", "s_dsc", "l_dsc"}


def is_int(v):
    # bool is a subclass of int in Python; reject it explicitly.
    return isinstance(v, int) and not isinstance(v, bool)


def describe(idx, entry):
    """Short human-readable identifier for an entry, for messages."""
    if isinstance(entry, dict):
        code = entry.get("code")
        dtc = entry.get("dtc")
        tag = []
        if code is not None:
            tag.append(f"code={code!r}")
        if dtc:
            tag.append(f"dtc={dtc!r}")
        ldsc = entry.get("l_dsc")
        if isinstance(ldsc, str) and ldsc:
            snippet = ldsc if len(ldsc) <= 40 else ldsc[:37] + "..."
            tag.append(f'l_dsc="{snippet}"')
        joined = ", ".join(tag) if tag else "<no identifiers>"
        return f"#{idx} ({joined})"
    return f"#{idx} ({type(entry).__name__})"


def validate(path):
    """Return (errors, warnings, summary) lists/dict for one file."""
    errors, warnings = [], []

    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
    except FileNotFoundError:
        return ([f"file not found: {path}"], [], {})
    except json.JSONDecodeError as e:
        return ([f"invalid JSON: {e}"], [], {})

    if not isinstance(data, list):
        return ([f"root must be a JSON array, got {type(data).__name__}"], [], {})

    code_seen = defaultdict(list)        # code value -> [indices]
    dtc_no_code_seen = defaultdict(list)  # dtc string (no code) -> [indices]
    n_by_code = n_by_dtc = n_empty_sdsc = 0

    for idx, entry in enumerate(data):
        if not isinstance(entry, dict):
            errors.append(f"{describe(idx, entry)}: entry is not an object")
            continue

        code = entry.get("code", None)
        dtc = entry.get("dtc", None)
        s_dsc = entry.get("s_dsc", None)
        l_dsc = entry.get("l_dsc", None)
        has_code_key = "code" in entry

        # --- l_dsc (the displayed text) is mandatory ---
        if not isinstance(l_dsc, str) or not l_dsc.strip():
            errors.append(f"{describe(idx, entry)}: missing or empty 'l_dsc'")

        # --- code validation ---
        code_usable = False
        if has_code_key and code is not None:
            if isinstance(code, str):
                if code == "":
                    errors.append(f"{describe(idx, entry)}: 'code' is an empty string "
                                  "(use null or omit it instead)")
                else:
                    errors.append(f"{describe(idx, entry)}: 'code' must be an integer, "
                                  f"got string {code!r}")
            elif not is_int(code):
                errors.append(f"{describe(idx, entry)}: 'code' must be an integer, "
                              f"got {type(code).__name__} {code!r}")
            else:
                code_usable = True
                if code == 0:
                    warnings.append(f"{describe(idx, entry)}: 'code' is 0 "
                                    "(creates m[\"0\"]; intended?)")
                code_seen[code].append(idx)

        # --- dtc validation ---
        dtc_usable = False
        if dtc is not None:
            if not isinstance(dtc, str):
                errors.append(f"{describe(idx, entry)}: 'dtc' must be a string, "
                              f"got {type(dtc).__name__} {dtc!r}")
            elif dtc != "":
                dtc_usable = True

        # --- must be matchable by something ---
        if not code_usable and not dtc_usable:
            errors.append(f"{describe(idx, entry)}: no usable key "
                          "(needs a numeric 'code' or a non-empty 'dtc')")

        # duplicate-dtc only matters when dtc is the effective key (no code)
        if dtc_usable and not code_usable:
            dtc_no_code_seen[dtc].append(idx)

        # --- soft checks ---
        if s_dsc is not None and not isinstance(s_dsc, str):
            warnings.append(f"{describe(idx, entry)}: 's_dsc' should be a string, "
                            f"got {type(s_dsc).__name__}")

        unknown = set(entry.keys()) - KNOWN_KEYS
        if unknown:
            warnings.append(f"{describe(idx, entry)}: unknown key(s) {sorted(unknown)}")

        # summary counters
        if code_usable:
            n_by_code += 1
        if dtc_usable:
            n_by_dtc += 1
        if not s_dsc:
            n_empty_sdsc += 1

    # --- cross-entry duplicate checks ---
    for code, idxs in code_seen.items():
        if len(idxs) > 1:
            warnings.append(f"duplicate 'code' {code} in entries {idxs} "
                            "(later overwrites earlier in lookup)")
    for dtc, idxs in dtc_no_code_seen.items():
        if len(idxs) > 1:
            warnings.append(f"duplicate 'dtc' {dtc!r} (no 'code') in entries {idxs} "
                            "(later overwrites earlier in lookup)")

    summary = {
        "entries": len(data),
        "by_code": n_by_code,
        "by_dtc": n_by_dtc,
        "empty_s_dsc": n_empty_sdsc,
    }
    return (errors, warnings, summary)


def main(argv=None):
    ap = argparse.ArgumentParser(description="Validate DTC description JSON file(s).")
    ap.add_argument("files", nargs="+", help="JSON file(s) to validate")
    ap.add_argument("--strict", action="store_true",
                    help="treat warnings as failures (non-zero exit)")
    ap.add_argument("-q", "--quiet", action="store_true",
                    help="only print files that have problems")
    args = ap.parse_args(argv)

    overall_ok = True
    for path in args.files:
        errors, warnings, summary = validate(path)
        ok = not errors and (not warnings or not args.strict)
        overall_ok = overall_ok and ok

        if args.quiet and not errors and not warnings:
            continue

        print(f"\n=== {path} ===")
        if summary:
            print(f"  entries: {summary['entries']}  "
                  f"(by code: {summary['by_code']}, by dtc: {summary['by_dtc']}, "
                  f"empty s_dsc: {summary['empty_s_dsc']})")
        for e in errors:
            print(f"  ERROR  : {e}")
        for w in warnings:
            print(f"  WARNING: {w}")
        status = "OK" if not errors and not warnings else (
            "FAIL" if errors else "OK (with warnings)")
        if args.strict and warnings and not errors:
            status = "FAIL (strict)"
        print(f"  -> {status}: {len(errors)} error(s), {len(warnings)} warning(s)")

    return 0 if overall_ok else 1


if __name__ == "__main__":
    sys.exit(main())
