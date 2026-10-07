#!/usr/bin/env python3
"""Smoke-check the generated WebAssembly bundle for GitHub Pages deployment."""

from __future__ import annotations

import sys
import hashlib
import json
from pathlib import Path


def require(path: Path) -> None:
    if not path.is_file():
        raise SystemExit(f"missing required artifact: {path}")
    if path.stat().st_size == 0:
        raise SystemExit(f"empty artifact: {path}")


def read_leb128(data: bytes, offset: int) -> tuple[int, int]:
    """Decode one unsigned LEB128 integer (7 bits per byte, high bit = more)."""
    value = shift = 0
    while True:
        if offset >= len(data):
            raise SystemExit("truncated WebAssembly section header")
        byte = data[offset]
        offset += 1
        value |= (byte & 0x7F) << shift
        if not byte & 0x80:
            return value, offset
        shift += 7


def custom_section_names(data: bytes) -> list[str]:
    """List custom-section names. After the 8-byte header a module is a
    sequence of (id byte, LEB128 size, payload); custom sections (id 0) begin
    their payload with a LEB128-length-prefixed name."""
    names, offset = [], 8
    while offset < len(data):
        section_id = data[offset]
        size, payload = read_leb128(data, offset + 1)
        end = payload + size
        if end > len(data):
            raise SystemExit("truncated WebAssembly section")
        if section_id == 0:
            length, start = read_leb128(data, payload)
            # The name (and its length prefix) must fit inside this section;
            # otherwise the slice would read into the next section or stop short.
            if start > end or start + length > end:
                raise SystemExit("custom section name runs past its section")
            names.append(data[start:start + length].decode("utf-8", errors="replace"))
        offset = end
    return names


def reject_debug_info(path: Path) -> None:
    # DWARF (.debug_*) and source maps are for local debugging only: they
    # multiply the download and embed absolute build-machine paths.
    debug = [name for name in custom_section_names(path.read_bytes())
             if name.startswith(".debug") or name in ("sourceMappingURL", "external_debug_info")]
    if debug:
        raise SystemExit(f"{path.name} ships debug sections {debug}; build release WASM without -g")


def main() -> int:
    web_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("build/web")
    stem = "solar-system-simulator"
    js = web_dir / f"{stem}.js"
    wasm = web_dir / f"{stem}.wasm"

    orbit_wasm = web_dir / 'catalog-orbits.wasm'
    lab_js = web_dir / 'learning-lab.mjs'
    lab_wasm = web_dir / 'learning-lab.wasm'
    manifest_path = web_dir / "build-info.json"
    for path in (js, wasm, orbit_wasm, lab_js, lab_wasm, manifest_path):
        require(path)

    manifest = json.loads(manifest_path.read_text())
    if manifest.get("schema") != 1 or not manifest.get("revision"):
        raise SystemExit("missing runtime source revision")
    for path in (js, wasm, orbit_wasm, lab_js, lab_wasm):
        if manifest.get("files", {}).get(path.name) != hashlib.sha256(path.read_bytes()).hexdigest():
            raise SystemExit(f"artifact checksum mismatch: {path.name}")

    js_text = js.read_text(encoding="utf-8", errors="replace")
    repo_root = Path(__file__).resolve().parents[1]
    main_text = (repo_root / "src" / "main.c").read_text(encoding="utf-8")

    if f"{stem}.wasm" not in js_text:
        raise SystemExit(f"{js} does not reference {stem}.wasm")

    if wasm.read_bytes()[:8] != b"\x00asm\x01\x00\x00\x00":
        raise SystemExit(f"{wasm} does not start with the WebAssembly magic and version bytes")
    if orbit_wasm.read_bytes()[:8] != b"\x00asm\x01\x00\x00\x00":
        raise SystemExit('invalid standalone catalog orbital module')
    if lab_wasm.read_bytes()[:8] != b"\x00asm\x01\x00\x00\x00":
        raise SystemExit('invalid comparison module')
    for module in (wasm, orbit_wasm, lab_wasm):
        reject_debug_info(module)
    lab_text = lab_js.read_text()
    for marker in ('learning-lab.wasm', '_lab_start', '_lab_advance', '_lab_point', '_lab_export_csv'):
        if marker not in lab_text:
            raise SystemExit(f'comparison module missing {marker}')

    for marker in ("solar_web_initial_canvas_width", "solar_web_initial_canvas_height", "solar_web_canvas_has_focus"):
        if marker not in main_text:
            raise SystemExit(f"src/main.c missing web sizing/focus boundary: {marker}")
    if "solar_web_report_state" not in main_text or "reportState" not in js_text:
        raise SystemExit("WebAssembly must report live simulation state to the Astro page")
    for marker in ("_solar_web_command", "_solar_web_experiment", "_solar_web_demo", "_solar_web_lesson", "_solar_web_export", "reportLabState", "downloadCsv", "clearBodies", "ccall", "addBody", "distanceM", "speedMps", "massKg", "radiusM", "massQuality", "radiusQuality", "achievedTimeScale", "pendingSeconds"):
        if marker not in js_text:
            raise SystemExit(f"WebAssembly missing inspection/control bridge: {marker}")

    if "const int screen_width = 1280" in main_text or "const int screen_height = 720" in main_text:
        raise SystemExit("src/main.c must not hardcode the web InitWindow() size to 1280x720")

    print(f"WASM artifacts OK in {web_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
