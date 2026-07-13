# PlatformIO pre: script — build Vite frontend and generate app_bundle.cpp/.h
import shutil
import subprocess
import sys
from os.path import join

Import("env")  # noqa: F821  — injected by SCons


def main():
    project_dir = env["PROJECT_DIR"]  # noqa: F821
    frontend = join(project_dir, "frontend")
    # The frontend is a pnpm workspace (pnpm-lock.yaml); fall back to npm.
    ext = ".cmd" if sys.platform == "win32" else ""
    if shutil.which("pnpm" + ext):
        pm = "pnpm" + ext
        install = [pm, "install", "--frozen-lockfile"]
    else:
        pm = "npm" + ext
        install = [pm, "install", "--legacy-peer-deps"]
    subprocess.check_call(install, cwd=frontend)
    subprocess.check_call([pm, "run", "build"], cwd=frontend)


main()
