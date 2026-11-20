#!/usr/bin/env python3
"""Analyze timing and payload patterns in PCAN and Battery-Emulator CAN logs.

The parser accepts PCAN-View TRC 1.x/2.x records and the text exported by the
Battery-Emulator web CAN logger. PCAN offsets are expressed in milliseconds;
web logger timestamps are expressed in seconds and are converted to ms.
"""

from __future__ import annotations

import argparse
import json
import math
import re
import statistics
import sys
from collections import Counter
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable, Sequence


DEFAULT_IDS = (0x053, 0x054, 0x055, 0x056, 0x118, 0x221, 0x3A1, 0x3C2)

WEB_LOG_RE = re.compile(
    r"^\s*\(\s*(?P<seconds>\d+(?:\.\d+)?)\s*\)\s+"
    r"(?P<direction>RX|TX)(?P<channel>\d*)\s+"
    r"(?P<id>(?:0x)?[0-9A-Fa-f]+)\s+\[(?P<dlc>\d+)\]"
    r"(?P<data>(?:\s+[0-9A-Fa-f]{2})*)\s*$",
    re.IGNORECASE,
)
PCAN_RECORD_RE = re.compile(
    r"^\s*(?P<number>\d+)\)\s+(?P<milliseconds>[+-]?\d+(?:\.\d+)?)\s+"
    r"(?P<direction>Rx|Tx)\s+(?P<rest>.+?)\s*$",
    re.IGNORECASE,
)
HEX_ID_RE = re.compile(r"^(?:0x)?[0-9A-Fa-f]{1,8}$")
BYTE_RE = re.compile(r"^[0-9A-Fa-f]{2}$")


@dataclass(frozen=True)
class CanFrame:
    timestamp_ms: float
    direction: str
    channel: str | None
    can_id: int
    dlc: int
    data: tuple[int, ...]
    line_number: int


@dataclass(frozen=True)
class CounterCandidate:
    field: str
    modulus: int
    confidence: float
    increment_every_frames: int


@dataclass(frozen=True)
class ChecksumCandidate:
    algorithm: str
    field: str
    confidence: float


@dataclass
class IdAnalysis:
    can_id: int
    frame_count: int
    first_timestamp_ms: float | None
    last_timestamp_ms: float | None
    interval_count: int
    average_interval_ms: float | None
    nominal_interval_ms: float | None
    nominal_interval_sample_percent: float | None
    median_interval_ms: float | None
    min_interval_ms: float | None
    max_interval_ms: float | None
    stdev_interval_ms: float | None
    dlcs: list[int]
    directions: list[str]
    channels: list[str]
    unique_payload_count: int
    changed_bytes: list[dict[str, float | int]]
    counter_candidates: list[CounterCandidate]
    checksum_candidates: list[ChecksumCandidate]
    behavior: str
    payloads: list[dict[str, float | int | str]]

    def to_dict(self) -> dict[str, object]:
        result = asdict(self)
        result["id"] = f"0x{self.can_id:03X}"
        del result["can_id"]
        return result


def parse_can_id(value: str) -> int:
    cleaned = value.strip().lower()
    if cleaned.startswith("0x"):
        cleaned = cleaned[2:]
    if not cleaned or not re.fullmatch(r"[0-9a-f]+", cleaned):
        raise ValueError(f"invalid CAN ID: {value!r}")
    can_id = int(cleaned, 16)
    if not 0 <= can_id <= 0x1FFFFFFF:
        raise ValueError(f"CAN ID out of range: {value!r}")
    return can_id


def _parse_pcan_rest(rest: str) -> tuple[str | None, int, int, tuple[int, ...]] | None:
    """Parse columns following PCAN's Rx/Tx field.

    TRC 1.x uses ``ID DLC DATA``. TRC 2.x normally adds a bus before the ID
    and flag columns (``BUS ID d R DLC DATA``). Finding the first ID-like
    token with at least three hex digits avoids mistaking the bus for an ID.
    """

    tokens = rest.split()
    if len(tokens) < 2:
        return None

    id_index: int | None = None
    for index, token in enumerate(tokens):
        normalized = token[2:] if token.lower().startswith("0x") else token
        if HEX_ID_RE.fullmatch(token) and len(normalized) >= 3:
            id_index = index
            break
    if id_index is None:
        return None

    channel = tokens[0] if id_index > 0 and tokens[0].isdigit() else None
    can_id = parse_can_id(tokens[id_index])

    dlc_index: int | None = None
    for index in range(id_index + 1, len(tokens)):
        token = tokens[index]
        if token.isdigit() and 0 <= int(token) <= 64:
            dlc_index = index
            break
    if dlc_index is None:
        return None

    dlc = int(tokens[dlc_index])
    data_tokens = tokens[dlc_index + 1 : dlc_index + 1 + dlc]
    if len(data_tokens) != dlc or any(not BYTE_RE.fullmatch(token) for token in data_tokens):
        return None
    return channel, can_id, dlc, tuple(int(token, 16) for token in data_tokens)


def parse_line(line: str, line_number: int) -> CanFrame | None:
    web_match = WEB_LOG_RE.match(line)
    if web_match:
        data = tuple(int(token, 16) for token in web_match.group("data").split())
        return CanFrame(
            timestamp_ms=float(web_match.group("seconds")) * 1000.0,
            direction=web_match.group("direction").upper(),
            channel=web_match.group("channel") or None,
            can_id=parse_can_id(web_match.group("id")),
            dlc=int(web_match.group("dlc")),
            data=data,
            line_number=line_number,
        )

    pcan_match = PCAN_RECORD_RE.match(line)
    if not pcan_match:
        return None
    parsed_rest = _parse_pcan_rest(pcan_match.group("rest"))
    if parsed_rest is None:
        return None
    channel, can_id, dlc, data = parsed_rest
    return CanFrame(
        timestamp_ms=float(pcan_match.group("milliseconds")),
        direction=pcan_match.group("direction").upper(),
        channel=channel,
        can_id=can_id,
        dlc=dlc,
        data=data,
        line_number=line_number,
    )


def parse_trace(lines: Iterable[str]) -> list[CanFrame]:
    frames: list[CanFrame] = []
    for line_number, line in enumerate(lines, start=1):
        frame = parse_line(line, line_number)
        if frame is not None:
            frames.append(frame)
    return frames


def read_trace(path: Path) -> list[CanFrame]:
    with path.open("r", encoding="utf-8-sig", errors="replace") as trace_file:
        return parse_trace(trace_file)


def _incrementing_modulus(
    values: Sequence[int], maximum_modulus: int
) -> tuple[int, float] | None:
    if len(values) < 4 or len(set(values)) < 2:
        return None
    transition_count = len(values) - 1
    for modulus in range(max(values) + 1, maximum_modulus + 1):
        if modulus < 2:
            continue
        matches = sum(next_value == (value + 1) % modulus for value, next_value in zip(values, values[1:]))
        confidence = matches / transition_count
        if confidence >= 0.95:
            return modulus, confidence
    return None


def _counter_for_values(
    values: Sequence[int], maximum_modulus: int
) -> tuple[int, float, int] | None:
    direct = _incrementing_modulus(values, maximum_modulus)
    if direct:
        return direct[0], direct[1], 1

    runs: list[tuple[int, int]] = []
    for value in values:
        if runs and runs[-1][0] == value:
            runs[-1] = (value, runs[-1][1] + 1)
        else:
            runs.append((value, 1))
    if len(runs) < 4:
        return None
    internal_lengths = [length for _, length in runs[1:-1]]
    if not internal_lengths:
        return None
    dwell, dwell_count = Counter(internal_lengths).most_common(1)[0]
    if dwell < 2 or dwell_count / len(internal_lengths) < 0.90:
        return None
    compressed = [value for value, _ in runs]
    compressed_result = _incrementing_modulus(compressed, maximum_modulus)
    if compressed_result:
        return compressed_result[0], compressed_result[1], dwell
    return None


def detect_checksums(frames: Sequence[CanFrame]) -> list[ChecksumCandidate]:
    if len(frames) < 4:
        return []
    common_dlc = Counter(frame.dlc for frame in frames).most_common(1)[0][0]
    if common_dlc < 2:
        return []
    payloads = [frame.data for frame in frames if frame.dlc == common_dlc]
    algorithms = (
        ("sum(data[0:-1]) + CAN_ID_low_byte, modulo 256", lambda data: sum(data[:-1]) + frames[0].can_id),
        ("sum(data[0:-1]), modulo 256", lambda data: sum(data[:-1])),
    )
    candidates: list[ChecksumCandidate] = []
    for name, calculate in algorithms:
        matches = sum(data[-1] == (calculate(data) & 0xFF) for data in payloads)
        confidence = matches / len(payloads)
        if confidence >= 0.98:
            candidates.append(ChecksumCandidate(name, f"byte[{common_dlc - 1}]", confidence))
    return candidates


def detect_counters(
    frames: Sequence[CanFrame], excluded_indices: set[int] | None = None
) -> list[CounterCandidate]:
    if len(frames) < 4:
        return []
    common_dlc = Counter(frame.dlc for frame in frames).most_common(1)[0][0]
    payloads = [frame.data for frame in frames if frame.dlc == common_dlc]
    candidates: list[CounterCandidate] = []

    excluded_indices = excluded_indices or set()
    for index in range(common_dlc):
        if index in excluded_indices:
            continue
        values = [payload[index] for payload in payloads]
        byte_result = _counter_for_values(values, 256)
        if byte_result:
            candidates.append(
                CounterCandidate(f"byte[{index}]", byte_result[0], byte_result[1], byte_result[2])
            )

        for nibble_name, nibble_values in (
            ("low_nibble", [value & 0x0F for value in values]),
            ("high_nibble", [value >> 4 for value in values]),
        ):
            nibble_result = _counter_for_values(nibble_values, 16)
            if not nibble_result:
                continue
            if byte_result and max(values) < 16 and byte_result[:2] == nibble_result[:2]:
                continue
            candidates.append(
                CounterCandidate(
                    f"byte[{index}].{nibble_name}",
                    nibble_result[0],
                    nibble_result[1],
                    nibble_result[2],
                )
            )
    return candidates


def _behavior(can_id: int, frame_count: int, intervals: Sequence[float]) -> str:
    if frame_count == 0:
        return "absent"
    if can_id == 0x054:
        if frame_count == 1:
            return "one-shot/manual-like"
        return "manual/repeated commands (not periodic)"
    if frame_count == 1:
        return "single frame (period unknown)"
    average = statistics.fmean(intervals)
    stdev = statistics.pstdev(intervals)
    coefficient_of_variation = stdev / average if average else math.inf
    median = statistics.median(intervals)
    nominal_intervals = [interval for interval in intervals if 0.8 * median <= interval <= 1.2 * median]
    nominal_fraction = len(nominal_intervals) / len(intervals)
    if coefficient_of_variation <= 0.10 or nominal_fraction >= 0.80:
        return f"periodic (~{median:.3f} ms)"
    if frame_count <= 5 or coefficient_of_variation >= 0.50:
        return "irregular/manual-like"
    return f"periodic with jitter (~{median:.3f} ms median)"


def analyze_id(frames: Sequence[CanFrame], can_id: int, payload_limit: int = 12) -> IdAnalysis:
    selected = sorted((frame for frame in frames if frame.can_id == can_id), key=lambda frame: frame.timestamp_ms)
    intervals = [
        current.timestamp_ms - previous.timestamp_ms
        for previous, current in zip(selected, selected[1:])
        if current.timestamp_ms >= previous.timestamp_ms
    ]
    median_interval = statistics.median(intervals) if intervals else None
    nominal_intervals = (
        [
            interval
            for interval in intervals
            if 0.8 * median_interval <= interval <= 1.2 * median_interval
        ]
        if median_interval is not None
        else []
    )

    payload_counter = Counter(frame.data for frame in selected)
    payload_first_seen: dict[tuple[int, ...], float] = {}
    payload_last_seen: dict[tuple[int, ...], float] = {}
    for frame in selected:
        payload_first_seen.setdefault(frame.data, frame.timestamp_ms)
        payload_last_seen[frame.data] = frame.timestamp_ms

    payloads: list[dict[str, float | int | str]] = []
    for payload, count in sorted(
        payload_counter.items(), key=lambda item: (-item[1], payload_first_seen[item[0]])
    )[:payload_limit]:
        payloads.append(
            {
                "data": " ".join(f"{byte:02X}" for byte in payload),
                "count": count,
                "first_timestamp_ms": payload_first_seen[payload],
                "last_timestamp_ms": payload_last_seen[payload],
            }
        )

    changed_bytes: list[dict[str, float | int]] = []
    if selected:
        max_dlc = max(frame.dlc for frame in selected)
        transition_count = len(selected) - 1
        for index in range(max_dlc):
            changes = sum(
                index >= len(previous.data)
                or index >= len(current.data)
                or previous.data[index] != current.data[index]
                for previous, current in zip(selected, selected[1:])
            )
            if changes:
                changed_bytes.append(
                    {
                        "index": index,
                        "transitions_changed": changes,
                        "percent": (100.0 * changes / transition_count) if transition_count else 0.0,
                    }
                )

    checksum_candidates = detect_checksums(selected)
    checksum_indices = {
        int(candidate.field.removeprefix("byte[").removesuffix("]"))
        for candidate in checksum_candidates
    }

    return IdAnalysis(
        can_id=can_id,
        frame_count=len(selected),
        first_timestamp_ms=selected[0].timestamp_ms if selected else None,
        last_timestamp_ms=selected[-1].timestamp_ms if selected else None,
        interval_count=len(intervals),
        average_interval_ms=statistics.fmean(intervals) if intervals else None,
        nominal_interval_ms=statistics.fmean(nominal_intervals) if nominal_intervals else None,
        nominal_interval_sample_percent=(100.0 * len(nominal_intervals) / len(intervals)) if intervals else None,
        median_interval_ms=median_interval,
        min_interval_ms=min(intervals) if intervals else None,
        max_interval_ms=max(intervals) if intervals else None,
        stdev_interval_ms=statistics.pstdev(intervals) if intervals else None,
        dlcs=sorted({frame.dlc for frame in selected}),
        directions=sorted({frame.direction for frame in selected}),
        channels=sorted({frame.channel for frame in selected if frame.channel is not None}),
        unique_payload_count=len(payload_counter),
        changed_bytes=changed_bytes,
        counter_candidates=detect_counters(selected, checksum_indices),
        checksum_candidates=checksum_candidates,
        behavior=_behavior(can_id, len(selected), intervals),
        payloads=payloads,
    )


def analyze_trace(
    frames: Sequence[CanFrame], can_ids: Sequence[int], payload_limit: int = 12
) -> list[IdAnalysis]:
    return [analyze_id(frames, can_id, payload_limit) for can_id in can_ids]


def _format_number(value: float | None, width: int = 0) -> str:
    rendered = "-" if value is None else f"{value:.3f}"
    return f"{rendered:>{width}}" if width else rendered


def format_text(path: Path, frames: Sequence[CanFrame], analyses: Sequence[IdAnalysis]) -> str:
    lines = [
        f"Trace: {path}",
        f"Parsed CAN data frames: {len(frames)}",
        "",
        "ID      Count   First ms      Last ms       Raw avg   Nominal avg   Median   Min / Max ms       DLC  Behavior",
        "------- ------- ------------ ------------ ------------ ------------- -------- ------------------ ----  --------",
    ]
    for item in analyses:
        min_max = (
            "-"
            if item.min_interval_ms is None
            else f"{item.min_interval_ms:.3f} / {item.max_interval_ms:.3f}"
        )
        lines.append(
            f"0x{item.can_id:03X} {item.frame_count:7d} "
            f"{_format_number(item.first_timestamp_ms, 12)} "
            f"{_format_number(item.last_timestamp_ms, 12)} "
            f"{_format_number(item.average_interval_ms, 12)} "
            f"{_format_number(item.nominal_interval_ms, 13)} "
            f"{_format_number(item.median_interval_ms, 8)} "
            f"{min_max:18} "
            f"{','.join(map(str, item.dlcs)) or '-':4}  {item.behavior}"
        )

    for item in analyses:
        lines.extend(["", f"0x{item.can_id:03X} payload analysis"])
        if not item.frame_count:
            lines.append("  No frames found.")
            continue
        lines.append(
            f"  Direction/channel: {','.join(item.directions) or '-'} / "
            f"{','.join(item.channels) or '-'}"
        )
        lines.append(f"  Unique payloads: {item.unique_payload_count}")
        if item.changed_bytes:
            changes = ", ".join(
                f"byte[{entry['index']}] {entry['percent']:.1f}%"
                for entry in item.changed_bytes
            )
            lines.append(f"  Changed between consecutive frames: {changes}")
        else:
            lines.append("  Changed between consecutive frames: none")
        if item.counter_candidates:
            counters = ", ".join(
                f"{candidate.field} mod {candidate.modulus}, increments every "
                f"{candidate.increment_every_frames} frame(s) ({candidate.confidence:.1%})"
                for candidate in item.counter_candidates
            )
            lines.append(f"  Rolling-counter candidates: {counters}")
        else:
            lines.append("  Rolling-counter candidates: none detected")
        if item.checksum_candidates:
            checksums = ", ".join(
                f"{candidate.field} = {candidate.algorithm} ({candidate.confidence:.1%})"
                for candidate in item.checksum_candidates
            )
            lines.append(f"  Checksum candidates: {checksums}")
        else:
            lines.append("  Checksum candidates: none detected")
        for payload in item.payloads:
            lines.append(
                f"  {payload['data'] or '<empty>'}  count={payload['count']} "
                f"first={payload['first_timestamp_ms']:.3f} ms "
                f"last={payload['last_timestamp_ms']:.3f} ms"
            )
        omitted = item.unique_payload_count - len(item.payloads)
        if omitted > 0:
            lines.append(f"  ... {omitted} additional payload(s); use --payload-limit to show more")
    return "\n".join(lines)


def _parse_ids(value: str) -> list[int]:
    try:
        parsed = [parse_can_id(token) for token in value.split(",") if token.strip()]
    except ValueError as error:
        raise argparse.ArgumentTypeError(str(error)) from error
    if not parsed:
        raise argparse.ArgumentTypeError("at least one CAN ID is required")
    return parsed


def build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Extract CAN frame timing, payload changes, and rolling-counter candidates."
    )
    parser.add_argument("trace", type=Path, help="PCAN .trc or Battery-Emulator CAN logger .txt file")
    parser.add_argument(
        "--ids",
        type=_parse_ids,
        default=list(DEFAULT_IDS),
        help="comma-separated hexadecimal CAN IDs (default: 053,054,055,056,118,221,3A1,3C2)",
    )
    parser.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    parser.add_argument(
        "--payload-limit",
        type=int,
        default=12,
        help="maximum payload variants shown per ID (default: 12)",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_argument_parser().parse_args(argv)
    if args.payload_limit < 1:
        print("error: --payload-limit must be at least 1", file=sys.stderr)
        return 2
    try:
        frames = read_trace(args.trace)
    except OSError as error:
        print(f"error: unable to read {args.trace}: {error}", file=sys.stderr)
        return 2
    if not frames:
        print(f"error: no supported CAN data records found in {args.trace}", file=sys.stderr)
        return 1

    analyses = analyze_trace(frames, args.ids, args.payload_limit)
    if args.json:
        output = {
            "trace": str(args.trace),
            "parsed_frame_count": len(frames),
            "ids": [analysis.to_dict() for analysis in analyses],
        }
        print(json.dumps(output, indent=2))
    else:
        print(format_text(args.trace, frames, analyses))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
