# PlatformIO pre: script — build Vite frontend and generate app_bundle.cpp/.h
import subprocess
import sys
from os.path import join

Import("env")  # noqa: F821  — injected by SCons


def main():
    project_dir = env["PROJECT_DIR"]  # noqa: F821
    frontend = join(project_dir, "frontend")
    npm = "npm.cmd" if sys.platform == "win32" else "npm"
    subprocess.check_call([npm, "install"], cwd=frontend)
    subprocess.check_call([npm, "run", "build"], cwd=frontend)


main()
