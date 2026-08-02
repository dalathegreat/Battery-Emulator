# PlatformIO pre-script: regenerate i18n headers and catalogs from the sheet
import os
import subprocess
import sys

Import("env")  # noqa: F821

subprocess.run(
    [sys.executable, os.path.join(env["PROJECT_DIR"], "tools", "i18n_gen.py")],
    check=True,
)
