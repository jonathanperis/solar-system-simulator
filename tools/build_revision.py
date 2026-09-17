"""Generate a build-only provenance header, preserving mtime when unchanged."""
import json
import os
import subprocess
import sys
from pathlib import Path

revision = os.environ.get("SOURCE_REVISION")
if not revision:
    result = subprocess.run(["git", "describe", "--always", "--dirty", "--abbrev=12"], capture_output=True, text=True)
    revision = result.stdout.strip() if result.returncode == 0 else "unversioned"
path = Path(sys.argv[1])
content = f"#define SOLAR_BUILD_REVISION {json.dumps(revision)}\n"
path.parent.mkdir(parents=True, exist_ok=True)
if not path.exists() or path.read_text() != content:
    path.write_text(content)
