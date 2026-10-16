#!/usr/bin/env python3
"""Generate a finite Battery-Emulator CAN replay for Tesla charge-mode testing.

The default replay intentionally omits 0x056 because the current test setup
already has an active 0x056 producer. It also omits every Battery-Emulator and
Ext. Module overlapping vehicle frame (0x118, 0x221, 0x3A1, and 0x3C2).
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence


@dataclass(frozen=True)
class OutgoingFrame:
    timestamp_s: float
    can_id: int
    data: tuple[int, ...]


INITIAL_053 = (0x54, 0x30, 0x84, 0xC3, 0x8F, 0x28, 0x46, 0x0D)
STARTING_053 = (0xD4, 0x30, 0x84, 0xC3, 0x8F, 0x28, 0x46, 0x0D)
STEADY_053 = (0xD4, 0x30, 0x84, 0xCB, 0x8F, 0x28, 0x46, 0x0D)
CHARGE_054 = (0x01, 0x00, 0x02, 0xE8, 0x03, 0x30, 0x87, 0x00)


def additive_checksum(can_id: int, values: Iterable[int]) -> int:
    return (can_id + sum(values)) & 0xFF


def payload_055(counter2: int, counter16: int, startup_initial: bool) -> tuple[int, ...]:
    state = 0 if startup_initial or counter2 != 2 else 1
    payload = (counter2, state, 0, 0, 0, 0, counter16)
    return payload + (additive_checksum(0x055, payload),)


def generate_replay(
    duration_s: float = 5.5,
    command_at_s: float = 3.75,
    initial_state_duration_s: float = 0.14,
    starting_state_duration_s: float = 3.0,
) -> list[OutgoingFrame]:
    if duration_s <= command_at_s:
        raise ValueError("duration must continue beyond the charge command")
    if min(command_at_s, initial_state_duration_s, starting_state_duration_s) < 0:
        raise ValueError("timings must not be negative")

    frames: list[OutgoingFrame] = []

    fast_counter = 0
    slow_counter = 1
    tick = 0
    while tick * 0.010 <= duration_s + 1e-9:
        timestamp = tick * 0.010
        frames.append(
            OutgoingFrame(
                timestamp,
                0x055,
                payload_055(fast_counter, slow_counter, timestamp < initial_state_duration_s),
            )
        )
        fast_counter = (fast_counter + 1) % 4
        if fast_counter == 0:
            slow_counter = (slow_counter + 1) % 16
        tick += 1

    steady_start_s = initial_state_duration_s + starting_state_duration_s
    tick = 0
    while 0.010 + tick * 0.020 <= duration_s + 1e-9:
        timestamp = 0.010 + tick * 0.020
        if timestamp < initial_state_duration_s:
            payload = INITIAL_053
        elif timestamp < steady_start_s:
            payload = STARTING_053
        else:
            payload = STEADY_053
        frames.append(OutgoingFrame(timestamp, 0x053, payload))
        tick += 1

    frames.append(OutgoingFrame(command_at_s, 0x054, CHARGE_054))
    priority = {0x055: 0, 0x053: 1, 0x054: 2}
    return sorted(frames, key=lambda frame: (frame.timestamp_s, priority[frame.can_id]))


def format_replay(frames: Sequence[OutgoingFrame]) -> str:
    return "\n".join(
        f"({frame.timestamp_s:.4f}) TX0 {frame.can_id:03X} [8] "
        + " ".join(f"{byte:02X}" for byte in frame.data)
        for frame in frames
    ) + "\n"


def build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Generate a finite Tesla charge-mode CAN replay")
    parser.add_argument("output", type=Path, help="destination .txt replay file")
    parser.add_argument("--duration", type=float, default=5.5, help="total replay duration in seconds")
    parser.add_argument(
        "--command-at",
        type=float,
        default=3.75,
        help="charge-command location in seconds (default: 3.75)",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_argument_parser().parse_args(argv)
    try:
        frames = generate_replay(args.duration, args.command_at)
    except ValueError as error:
        raise SystemExit(f"error: {error}") from error
    args.output.write_text(format_replay(frames), encoding="utf-8")
    print(f"Wrote {len(frames)} frames to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
