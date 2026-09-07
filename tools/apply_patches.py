"""
Apply all patches from tools/patches/ to the ESP-IDF source tree before the
pioarduino HybridCompile, so driver fixes are compiled into the linked
libraries. Fails the build if a patch cannot be applied, so stale or broken
patches get tidied up instead of silently ignored.

If this fails on a patch, it is likely that ESP-IDF has since been updated and
the patch can be removed.

"""

import subprocess
import sys
from pathlib import Path

Import("env")  # noqa: F821  (provided by SCons)

PATCHES_DIR = Path(env.subst("$PROJECT_DIR")) / "tools" / "patches"


def main():
    platform = env.PioPlatform()
    espidf = platform.get_package_dir("framework-espidf")
    if not espidf or not PATCHES_DIR.is_dir():
        print("apply_patches: no framework-espidf or tools/patches, skipping")
        return

    applied_any = False
    for patch in sorted(PATCHES_DIR.glob("*.patch")):
        data = patch.read_bytes()

        # Already applied -> skip (keeps things idempotent across rebuilds).
        r = subprocess.run(
            ["git", "apply", "--reverse", "--check", "-"],
            cwd=espidf, input=data, capture_output=True,
        )
        if r.returncode == 0:
            print(f"apply_patches: {patch.name} already applied")
            continue

        r = subprocess.run(["git", "apply", "-"], cwd=espidf, input=data, capture_output=True)
        if r.returncode != 0:
            print(f"apply_patches: FAILED to apply {patch.name}", file=sys.stderr)
            print(r.stderr.decode(), file=sys.stderr)
            print("Fix the patch (or remove it) in tools/patches/, then rebuild.", file=sys.stderr)
            sys.exit(1)

        applied_any = True
        print(f"apply_patches: applied {patch.name}")


main()
