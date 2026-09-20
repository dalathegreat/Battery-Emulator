#!/usr/bin/env python3

"""
Clean/regenerate the ESP-IDF managed fb_gfx component.

Run from the repository root:

    python fix_fb_gfx.py

The script deliberately does NOT modify:
    ~/.platformio/packages/framework-arduinoespressif32

It only works with the project's:
    managed_components/
    dependencies.lock
"""

from pathlib import Path
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parent

LOCKFILE = ROOT / "dependencies.lock"
MANAGED = ROOT / "managed_components"
FB_GFX = MANAGED / "espressif__fb_gfx"

# This is the source currently used by the project.
FB_GFX_URL = (
    "https://github.com/espressif/esp32-arduino-lib-builder.git"
)

FB_GFX_REV = "6671d0bd65cdb9d4cc1001b759e8610de945a8d5"


def run(cmd, check=True):
    print("\n>", " ".join(str(x) for x in cmd))
    return subprocess.run(cmd, cwd=ROOT, check=check)


def git_available():
    try:
        run(["git", "--version"])
        return True
    except Exception:
        return False


def backup_lock():
    if not LOCKFILE.exists():
        print("No dependencies.lock found.")
        return None

    backup = LOCKFILE.with_suffix(".lock.bak")

    shutil.copy2(LOCKFILE, backup)

    print(f"Backed up:")
    print(f"  {LOCKFILE}")
    print(f"to:")
    print(f"  {backup}")

    return backup


def remove_fb_gfx():
    if FB_GFX.exists():
        print(f"\nRemoving generated component:")
        print(f"  {FB_GFX}")

        shutil.rmtree(FB_GFX)

    else:
        print("\nfb_gfx component directory does not exist.")


def remove_component_lock():
    """
    Remove only the fb_gfx entry from dependencies.lock.

    The ESP-IDF component manager will recreate the entry when
    the project is configured.
    """

    if not LOCKFILE.exists():
        print("dependencies.lock does not exist; nothing to edit.")
        return

    text = LOCKFILE.read_text(encoding="utf-8")

    marker = "  espressif/fb_gfx:\n"

    if marker not in text:
        print("No fb_gfx entry found in dependencies.lock.")
        return

    start = text.index(marker)

    # Find next top-level component.
    remainder = text[start + len(marker):]

    next_component = remainder.find("\n  espressif/")

    if next_component == -1:
        print("Could not safely determine fb_gfx block boundary.")
        print("Leaving dependencies.lock unchanged.")
        return

    end = start + len(marker) + next_component + 1

    new_text = text[:start] + text[end:]

    # Remove fb_gfx from direct_dependencies too.
    new_text = new_text.replace(
        "- espressif/fb_gfx\n",
        ""
    )

    LOCKFILE.write_text(new_text, encoding="utf-8")

    print("Removed fb_gfx from dependencies.lock.")


def verify_git_revision():
    if not git_available():
        return

    print("\nChecking upstream fb_gfx revision...")

    result = subprocess.run(
        [
            "git",
            "ls-remote",
            FB_GFX_URL,
            FB_GFX_REV,
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
    )

    if result.returncode != 0:
        print("WARNING: Could not query upstream repository.")
        print(result.stderr)
        return

    if FB_GFX_REV in result.stdout:
        print("OK: fb_gfx revision exists upstream:")
        print(f"  {FB_GFX_REV}")
    else:
        print("WARNING: requested fb_gfx revision was not found upstream.")
        print(result.stdout)


def main():
    print("=" * 70)
    print("Battery Emulator - fb_gfx repair")
    print("=" * 70)

    print(f"\nRepository:")
    print(f"  {ROOT}")

    # Safety check.
    if not (ROOT / "platformio.ini").exists():
        print("\nERROR: platformio.ini was not found.")
        print("Run this script from the Battery-Emulator repository.")
        sys.exit(1)

    verify_git_revision()

    backup_lock()

    remove_fb_gfx()

    remove_component_lock()

    print("\n" + "=" * 70)
    print("Cleanup complete.")
    print("=" * 70)

    print(
        """
Next step:

    pio run -e waveshare_330

The ESP-IDF component manager should download fb_gfx again and
regenerate dependencies.lock.

After the build finishes, run:

    git diff -- dependencies.lock

If dependencies.lock changed, inspect the fb_gfx entry.

IMPORTANT:
Do NOT modify anything under:

    ~/.platformio/packages/framework-arduinoespressif32

The script intentionally leaves the PlatformIO framework untouched.
"""
    )


if __name__ == "__main__":
    main()
