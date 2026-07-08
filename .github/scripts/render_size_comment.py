#!/usr/bin/env python3
"""Render a firmware-size PR comment from base-side and PR-side size reports.

Consumed by .github/workflows/measure-firmware-size.yml. Each side is a
directory (potentially with subdirectories from `merge-multiple: true`)
containing one `<pio_env>.size.json` per board built by
`.github/workflows/compile-common-image.yml`.
"""
import argparse
import json
import pathlib
import sys
from dataclasses import dataclass

from size_report import BoardSize, MemoryBytes


STICKY_MARKER = "<!-- firmware-size-report -->"
WARN_FILL_THRESHOLD = 0.90  # append ⚠️ when PR fill > 90% AND flash grew


@dataclass
class RowCells:
    """One rendered markdown cell per column, plus the table's header/alignment.

    The column schema lives entirely on this class: field order, header
    labels, alignment markers, and body formatting are all in the three
    adjacent methods below. Adding or removing a column means editing
    a field, a header entry, an alignment entry, and a render entry
    """
    board: str
    env: str
    base_flash: str
    pr_flash: str
    max_flash: str
    pr_pct: str
    delta_flash: str
    pr_ram: str
    delta_ram: str

    @staticmethod
    def header() -> tuple[str, str]:
        """Return (header row, alignment row) for the markdown table."""
        return (
            "| Board | Env | Base flash | PR flash | Max | PR % | Δ flash | PR RAM | Δ RAM |",
            "| - | - | -: | -: | -: | -: | -: | -: | -: |",
        )

    def render(self) -> str:
        """Format as a single markdown table row."""
        return (
            f"| {self.board} | {self.env} "
            f"| {self.base_flash} | {self.pr_flash} | {self.max_flash} "
            f"| {self.pr_pct} | {self.delta_flash} "
            f"| {self.pr_ram} | {self.delta_ram} |"
        )


def load_size_reports(root: str) -> dict[str, BoardSize]:
    """Return {pio_env: BoardSize} for every *.size.json under root."""
    out: dict[str, BoardSize] = {}
    p = pathlib.Path(root)
    if not p.exists():
        return out
    for f in p.rglob("*.size.json"):
        try:
            with f.open(encoding="utf-8") as fh:
                doc = json.load(fh)
            board = BoardSize.from_dict(doc)
        except (OSError, json.JSONDecodeError, KeyError) as exc:
            print(f"render_size_comment: skipping {f}: {exc}", file=sys.stderr)
            continue
        out[board.pio_env] = board
    return out


def short(sha: str) -> str:
    return (sha or "")[:7] or "?"


def fmt_bytes(n: int | None) -> str:
    if n is None:
        return "—"
    return f"{n:,}"


def fmt_delta(delta: int | None) -> str:
    if delta is None:
        return ""
    if delta == 0:
        return "0"
    return f"{delta:+,}"


def fmt_pct(usage: MemoryBytes | None) -> str:
    if usage is None or not usage.total:
        return "—"
    return f"{(usage.used / usage.total) * 100:.2f}%"


def sort_key(env: str, pr: BoardSize | None) -> tuple[float, str]:
    """Sort by PR-side flash fill % desc; envs without PR data go last."""
    if pr and pr.flash and pr.flash.total:
        return (-(pr.flash.used / pr.flash.total), env)
    return (float("inf"), env)


def flash_delta_cell(base: MemoryBytes | None, pr: MemoryBytes | None) -> str:
    """Signed flash delta. ⚠️ if tight (>90%) and grew; ⬇️ if the board shrank."""
    if base is None or pr is None:
        return "—"
    delta = pr.used - base.used
    cell = fmt_delta(delta)
    if delta > 0 and pr.total and (pr.used / pr.total) > WARN_FILL_THRESHOLD:
        cell += " ⚠️"
    elif delta < 0:
        cell += " ⬇️"
    return cell


def ram_delta_cell(base: MemoryBytes | None, pr: MemoryBytes | None) -> str:
    """Signed RAM delta, or — if either side is missing.

    No fill % for RAM: heap allocations dominate runtime RAM anyway, so
    absolute deltas are what reviewers care about.
    """
    if base is None or pr is None:
        return "—"
    return fmt_delta(pr.used - base.used)


def _row_pr_missing(env: str, base: BoardSize | None) -> RowCells:
    """PR-side build failed for this env — show base numbers, blank PR side."""
    flash = base.flash if base else None
    return RowCells(
        board=(base.board_name if base else env),
        env=f"`{env}`",
        base_flash=fmt_bytes(flash.used if flash else None),
        pr_flash="build failed",
        max_flash=fmt_bytes(flash.total if flash else None),
        pr_pct="—",
        delta_flash="—",
        pr_ram="—",
        delta_ram="—",
    )


def _row_base_missing(env: str, pr: BoardSize) -> RowCells:
    """No base report — new board, or base build failed. Mark deltas as (new)."""
    return RowCells(
        board=pr.board_name,
        env=f"`{env}`",
        base_flash="—",
        pr_flash=fmt_bytes(pr.flash.used if pr.flash else None),
        max_flash=fmt_bytes(pr.flash.total if pr.flash else None),
        pr_pct=fmt_pct(pr.flash),
        delta_flash="(new)",
        pr_ram=fmt_bytes(pr.ram.used if pr.ram else None),
        delta_ram="(new)",
    )


def _row_normal(env: str, base: BoardSize, pr: BoardSize) -> RowCells:
    """Both builds succeeded — show sizes, deltas, and flag if tight+growing."""
    return RowCells(
        board=pr.board_name,
        env=f"`{env}`",
        base_flash=fmt_bytes(base.flash.used if base.flash else None),
        pr_flash=fmt_bytes(pr.flash.used if pr.flash else None),
        max_flash=fmt_bytes(pr.flash.total if pr.flash else None),
        pr_pct=fmt_pct(pr.flash),
        delta_flash=flash_delta_cell(base.flash, pr.flash),
        pr_ram=fmt_bytes(pr.ram.used if pr.ram else None),
        delta_ram=ram_delta_cell(base.ram, pr.ram),
    )


def render_row(env: str, base: BoardSize | None, pr: BoardSize | None) -> str:
    """Dispatch to the right case builder and format the resulting row."""
    if pr is None:
        cells = _row_pr_missing(env, base)
    elif base is None:
        cells = _row_base_missing(env, pr)
    else:
        cells = _row_normal(env, base, pr)
    return cells.render()


def render(
    base: dict[str, BoardSize],
    pr: dict[str, BoardSize],
    base_branch: str,
    base_sha: str,
    head_sha: str,
) -> str:
    envs = sorted(set(base) | set(pr), key=lambda e: sort_key(e, pr.get(e)))

    lines: list[str] = [
        STICKY_MARKER,
        "## Firmware size report",
        "",
    ]

    if not base:
        lines.append(
            f"> No base-branch size reports available for `{base_branch}` yet; "
            "showing absolute sizes only. Deltas will appear once a compile run "
            "on the base branch has published size artifacts."
        )
        lines.append("")
    else:
        lines.append(
            f"Base: `{short(base_sha)}` on `{base_branch}` · This PR: `{short(head_sha)}`"
        )
        lines.append("")

    header, sep = RowCells.header()
    lines.append(header)
    lines.append(sep)

    for env in envs:
        lines.append(render_row(env, base.get(env), pr.get(env)))

    lines.append("")
    lines.append(
        "⚠️ = board already >90% full and flash grew. "
        "Sorted by PR flash fill % (tightest first)."
    )
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--pr-dir", required=True)
    ap.add_argument("--base-dir", required=True)
    ap.add_argument("--base-branch", required=True)
    ap.add_argument("--base-sha", required=True)
    ap.add_argument("--head-sha", required=True)
    args = ap.parse_args()

    base = load_size_reports(args.base_dir)
    pr = load_size_reports(args.pr_dir)

    if not pr and not base:
        # Nothing to say. Emit a minimal note so the sticky comment
        # updates in place instead of leaving stale numbers.
        body = (
            f"{STICKY_MARKER}\n"
            "## Firmware size report\n\n"
            "> No size reports found in either the PR build or the base build.\n"
        )
    else:
        body = render(base, pr, args.base_branch, args.base_sha, args.head_sha)

    sys.stdout.buffer.write(body.encode("utf-8"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
