"""Normalize downloaded/local runtime assets into Astro's public directory."""
import shutil
import sys
from pathlib import Path

from check_wasm_artifacts import main as check_artifacts

source, target = map(Path, sys.argv[1:3])
target.mkdir(parents=True, exist_ok=True)
for name in ("solar-system-simulator.js", "solar-system-simulator.wasm", "catalog-orbits.wasm", "learning-lab.mjs", "learning-lab.wasm", "build-info.json"):
    matches = list(source.rglob(name))
    if len(matches) != 1:
        raise SystemExit(f"expected one {name} in {source}, found {len(matches)}")
    destination = target / name
    if matches[0].resolve() != destination.resolve():
        shutil.copyfile(matches[0], destination)
sys.argv = [sys.argv[0], str(target)]
check_artifacts()
