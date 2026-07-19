"""Shared dataclasses for firmware size reports.

Both `parse_size.py` (producer, writes JSON) and `render_size_comment.py`
(consumer, reads JSON) speak this schema. Keeping it in one file means the
consumer doesn't have to re-derive the shape from raw dicts.

The on-disk format is `dataclasses.asdict(board)` — each `*.size.json`
artifact deserialises back into a `BoardSize` via `BoardSize.from_dict`.
"""
from dataclasses import dataclass


@dataclass
class MemoryBytes:
    used: int
    total: int

    @classmethod
    def from_dict(cls, d: dict | None) -> "MemoryBytes | None":
        if not d:
            return None
        return cls(used=d["used"], total=d["total"])


@dataclass(frozen=True)
class Toolchain:
    """Identity of the toolchain a board was built with.

    Frozen so instances are hashable — the consumer collects the distinct
    toolchains across a comparison into a set and warns if there's more than one.
    """
    pio_core: str          # PlatformIO Core version, e.g. "6.1.18"
    platform: str          # resolved platform-espressif32, e.g. "55.03.39"

    @classmethod
    def from_dict(cls, d: dict) -> "Toolchain":
        return cls(pio_core=d["pio_core"], platform=d["platform"])

    def label(self) -> str:
        return f"PlatformIO Core {self.pio_core} · platform-espressif32 {self.platform}"


@dataclass
class BoardSize:
    schema_version: int
    pio_env: str
    board_name: str
    sha: str
    flash: MemoryBytes | None
    ram: MemoryBytes | None
    toolchain: Toolchain

    @classmethod
    def from_dict(cls, d: dict) -> "BoardSize":
        return cls(
            schema_version=d["schema_version"],
            pio_env=d["pio_env"],
            board_name=d["board_name"],
            sha=d["sha"],
            flash=MemoryBytes.from_dict(d.get("flash")),
            ram=MemoryBytes.from_dict(d.get("ram")),
            toolchain=Toolchain.from_dict(d["toolchain"]),
        )
