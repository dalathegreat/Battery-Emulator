"""
Build the frontend during the PlatformIO build. 

The frontend includes the datalayer struct layouts extracted from the
battery/datalayer object files, so needs to happen after those are built, but
before the webserver (which includes the frontend bytes as a header) is build
and the firmware linked.

"""

import fnmatch
import glob
import os
import shutil
import subprocess
import sys

Import("env")

PROJECT_DIR = env.subst("$PROJECT_DIR")
PROJECT_SRC_DIR = env.subst("$PROJECT_SRC_DIR")
BUILD_SRC_DIR = env.subst("$BUILD_SRC_DIR")

EXTRACTOR = os.path.join(PROJECT_DIR, "tools", "extract_datalayer_info_structures.py")
FRONTEND_DIR = os.path.join(PROJECT_DIR, "frontend")
DATALAYER_OUT = os.path.join(FRONTEND_DIR, "src", "ext", "datalayer")
FRONTEND_H = os.path.join(PROJECT_SRC_DIR, "src", "devboard", "webserver", "frontend.h")
STAMP = os.path.join(env.subst("$BUILD_DIR"), ".datalayer_extracted.stamp")


def _frontend_src_nodes(pattern):
    """
    Loop over every file in frontend/src/ matching `pattern` and return SCons
    nodes for them.
    """
    nodes = []
    src_dir = os.path.join(FRONTEND_DIR, "src")
    for root, _dirs, filenames in os.walk(src_dir):
        for name in sorted(filenames):
            if fnmatch.fnmatch(name, pattern):
                nodes.extend(env.Glob(os.path.join(root, name)))
    return nodes


def _run(cmd, cwd=None):
    print("> " + " ".join(cmd))
    proc = subprocess.run(cmd, cwd=cwd)
    if proc.returncode != 0:
        raise RuntimeError(
            "build_frontend: '%s' failed with exit code %d"
            % (" ".join(cmd), proc.returncode)
        )


def _snapshot_datalayer():
    """
    Return a map of filename -> content for every generated .ts file.
    """
    snap = {}
    if os.path.isdir(DATALAYER_OUT):
        for name in sorted(os.listdir(DATALAYER_OUT)):
            path = os.path.join(DATALAYER_OUT, name)
            if os.path.isfile(path):
                with open(path, "rb") as f:
                    snap[name] = f.read()
    return snap


def _datalayer_hash(snap):
    """
    Return a content hash of the extracted structs.
    """
    import hashlib

    h = hashlib.sha256()
    for name, content in snap.items():
        h.update(name.encode())
        h.update(content)
    return h.hexdigest()


def _extract_datalayer(target, source, env):
    """
    Run the datalayer extractor on the object files, and write a stamp.
    """
    objs = [str(s.abspath) for s in source if str(s.abspath).endswith(".o")]
    snap = _snapshot_datalayer()
    before = _datalayer_hash(snap)
    _run([sys.executable, EXTRACTOR, DATALAYER_OUT] + objs)
    after = _datalayer_hash(_snapshot_datalayer())
    stamp = str(target[0])
    # The stamp content is the hash of the extracted structs, so SCons only
    # re-runs the (slower) frontend build when the structs actually changed.
    with open(stamp, "w") as f:
        f.write("datalayer structs hash: %s\n" % after)
    if before != after:
        print("build_frontend: datalayer structs changed, frontend will rebuild")


def _build_frontend(target, source, env):
    _run(["bun", "run", "build"], cwd=FRONTEND_DIR)


def _stale_frontend_sources():
    """
    Find frontend sources newer than the checked-in frontend.h.

    Returns None when frontend.h is missing entirely, otherwise a sorted list
    of relative paths whose mtime is newer than frontend.h's.
    """
    try:
        header_mtime = os.path.getmtime(FRONTEND_H)
    except OSError:
        return None
    stale = []
    src_dir = os.path.join(FRONTEND_DIR, "src")
    for root, _dirs, filenames in os.walk(src_dir):
        for name in sorted(filenames):
            if not (fnmatch.fnmatch(name, "*.css") or fnmatch.fnmatch(name, "*.ts")
                    or fnmatch.fnmatch(name, "*.tsx")):
                continue
            path = os.path.join(root, name)
            # +1s tolerance: SCons' content decider never rebuilt this in the
            # checked-in state, and we only want to flag edits since the header.
            if os.path.getmtime(path) > header_mtime + 1:
                stale.append(os.path.relpath(path, PROJECT_DIR))
    return stale


def _warn_missing_bun():
    """
    Print a loud, hard-to-miss warning that the frontend chain is skipped.
    """
    color = "\033[1;31m" if sys.stdout.isatty() else ""
    reset = "\033[0m" if sys.stdout.isatty() else ""
    line = "=" * 78
    print()
    print(color + line + reset)
    print(color + "*** WARNING: 'bun' not found on PATH - the frontend will NOT be rebuilt ***" + reset)
    print(color + line + reset)
    print("  " + os.path.relpath(FRONTEND_H, PROJECT_DIR) + " will be used as-is.")
    print("  Changes under frontend/ (CSS, TSX, TS, ...) will NOT appear in the firmware.")
    stale = _stale_frontend_sources()
    if stale is None:
        print("  " + os.path.relpath(FRONTEND_H, PROJECT_DIR) + " is MISSING - the firmware may not compile.")
    elif stale:
        print("  %d frontend source(s) are NEWER than frontend.h and will be ignored:" % len(stale))
        for path in stale[:5]:
            print("    - " + path)
        if len(stale) > 5:
            print("    - ... and %d more" % (len(stale) - 5))
    print()
    print("  Fix: install bun (https://bun.sh) or add it to PATH, e.g.:")
    print("      export PATH=\"$HOME/.bun/bin:$PATH\"")
    print("  To make this a hard build error instead of a warning, run with:")
    print("      BUILD_FRONTEND_STRICT=1")
    print(color + line + reset)
    print()


if not shutil.which("bun"):
    _warn_missing_bun()
    if os.environ.get("BUILD_FRONTEND_STRICT") == "1":
        raise RuntimeError(
            "build_frontend: 'bun' not found in PATH and BUILD_FRONTEND_STRICT=1 - "
            "refusing to build with a potentially stale frontend.h"
        )
else:

    def _object_nodes(rel_dir):
        """
        Return SCons nodes for the .o files of every .cpp in src/<rel_dir>.

        These match the targets the platform's BuildSources() creates
        ($BUILD_SRC_DIR/<rel_dir>/<name>.cpp.o), so the platform builder and
        the frontend chain share the same nodes.
        """
        nodes = []
        src_dir = os.path.join(PROJECT_SRC_DIR, rel_dir)
        for cpp in sorted(glob.glob(os.path.join(src_dir, "*.cpp"))):
            obj = os.path.join(
                BUILD_SRC_DIR, rel_dir, os.path.basename(cpp) + ".o"
            )
            nodes.append(env.File(obj))
        return nodes

    # 1. Extract the DATALAYER_INFO_*/Battery struct layouts from the object
    #    files into frontend/src/ext/datalayer/*.ts.
    extract_sources = _object_nodes("src/battery") + _object_nodes("src/datalayer")
    extract_sources.append(env.File(EXTRACTOR))
    stamp = env.Command(
        STAMP,
        extract_sources,
        env.Action(_extract_datalayer, "Extracting datalayer structs from object files"),
    )

    # 2. Rebuild the frontend, which regenerates frontend.h (only rewritten by
    #    vite.config.ts when its content actually changed).
    frontend_sources = [stamp[0]]
    for pattern in (
        "index.html",
        "package.json",
        "bun.lock",
        "bunfig.toml",
        "vite.config.ts",
        "tsconfig.json",
        "tsconfig.app.json",
        "tsconfig.node.json",
    ):
        frontend_sources += env.Glob(os.path.join(FRONTEND_DIR, pattern))
    frontend_sources += _frontend_src_nodes("*.ts")
    frontend_sources += _frontend_src_nodes("*.tsx")
    frontend_sources += _frontend_src_nodes("*.css")


    # The frontend bakes the firmware version (from version_autogen.h, written
    # by the tools/identify_build.py pre-script) into the bundle. Depending on
    # it here keeps the burnt-in version in sync with the backend: any version
    # change (new git SHA/tag/branch) rebuilds the frontend even if no
    # frontend source changed. identify_build.py only rewrites the header when
    # its content changes, so unchanged versions don't trigger rebuilds.
    version_autogen = os.path.join(
        PROJECT_DIR, "Software", "src", "devboard", "utils", "version_autogen.h"
    )
    if os.path.exists(version_autogen):
        frontend_sources += [env.File(version_autogen)]

    frontend_h = env.Command(
        FRONTEND_H,
        frontend_sources,
        env.Action(_build_frontend, "Building frontend (bun run build)"),
    )

    # 3. SCons' #include scanner already rebuilds webserver_new.cpp.o when
    #    frontend.h changes. Also pin the program to frontend.h explicitly so
    #    the header is always in place before linking.
    env.Depends(env.subst("$BUILD_DIR/$PROGNAME"), frontend_h[0])
