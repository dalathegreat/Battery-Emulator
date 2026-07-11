#!/usr/bin/env python3
"""Parse `pio run -t checkprogsize` output into a per-env size report.

Consumed by .github/workflows/measure-firmware-size.yml to produce a
firmware-size PR comment without rebuilding.

checkprogsize output looks like:

    RAM:   [==        ]  13.9% (used 45678 bytes from 327680 bytes)
    Flash: [======    ]  62.8% (used 1234567 bytes from 1966080 bytes)
"""
import argparse
import json
import re
import sys
from dataclasses import asdict

from size_report import BoardSize, MemoryBytes, Toolchain


RAM_RE = re.compile(r"^RAM:.*used\s+(\d+)\s+bytes\s+from\s+(\d+)\s+bytes", re.MULTILINE)
FLASH_RE = re.compile(r"^Flash:.*used\s+(\d+)\s+bytes\s+from\s+(\d+)\s+bytes", re.MULTILINE)


def _parse_usage(regex: re.Pattern[str], text: str) -> MemoryBytes | None:
    m = regex.search(text)
    if m is None:
        return None
    return MemoryBytes(used=int(m.group(1)), total=int(m.group(2)))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--pio-env", required=True)
    ap.add_argument("--board-name", required=True)
    ap.add_argument("--sha", required=True)
    ap.add_argument(
        "--pio-system-info",
        required=True,
        help="raw JSON from `pio system info --json-output`",
    )
    ap.add_argument(
        "--platform-json",
        required=True,
        help="contents of the installed platform.json",
    )
    args = ap.parse_args()

    text = sys.stdin.read()
    ram = _parse_usage(RAM_RE, text)
    flash = _parse_usage(FLASH_RE, text)
    if ram is None and flash is None:
        # Neither line found — checkprogsize probably didn't run. Fail loudly
        # so a broken producer surfaces at PR time instead of silently
        # emitting empty size reports.
        print("parse_size: no Flash: or RAM: lines found on stdin", file=sys.stderr)
        return 2

    toolchain = Toolchain(
        pio_core=json.loads(args.pio_system_info)["core_version"]["value"],
        platform=json.loads(args.platform_json)["version"],
    )

    doc = BoardSize(
        schema_version=1,
        pio_env=args.pio_env,
        board_name=args.board_name,
        sha=args.sha,
        flash=flash,
        ram=ram,
        toolchain=toolchain,
    )
    with open(f"{args.pio_env}.size.json", "w", encoding="utf-8") as f:
        json.dump(asdict(doc), f, indent=2)
        f.write("\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
