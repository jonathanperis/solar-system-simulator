"""Attach revision and byte-level provenance to the compiled C artifacts."""
import hashlib
import json
import sys
from pathlib import Path

directory = Path(sys.argv[1])
header = Path(sys.argv[2]).read_text().strip()
revision = json.loads(header.removeprefix("#define SOLAR_BUILD_REVISION "))
names = ("solar-system-simulator.js", "solar-system-simulator.wasm", "catalog-orbits.wasm", "learning-lab.mjs", "learning-lab.wasm")
manifest = {"schema": 1, "revision": revision,
            "files": {name: hashlib.sha256((directory / name).read_bytes()).hexdigest() for name in names}}
(directory / "build-info.json").write_text(json.dumps(manifest, indent=2) + "\n")
