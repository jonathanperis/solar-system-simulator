#!/usr/bin/env python3
"""Smoke-check the generated WebAssembly bundle for GitHub Pages deployment."""

from __future__ import annotations

import sys
from pathlib import Path


def require(path: Path) -> None:
    if not path.is_file():
        raise SystemExit(f"missing required artifact: {path}")
    if path.stat().st_size == 0:
        raise SystemExit(f"empty artifact: {path}")


def main() -> int:
    web_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("build/web")
    stem = "solar-system-simulator"
    js = web_dir / f"{stem}.js"
    wasm = web_dir / f"{stem}.wasm"

    for path in (js, wasm):
        require(path)

    js_text = js.read_text(encoding="utf-8", errors="replace")
    repo_root = Path(__file__).resolve().parents[1]
    main_text = (repo_root / "src" / "main.c").read_text(encoding="utf-8")

    if f"{stem}.wasm" not in js_text:
        raise SystemExit(f"{js} does not reference {stem}.wasm")

    if wasm.read_bytes()[:8] != b"\x00asm\x01\x00\x00\x00":
        raise SystemExit(f"{wasm} does not start with the WebAssembly magic and version bytes")

    for marker in ("solar_web_initial_canvas_width", "solar_web_initial_canvas_height", "solar_web_canvas_has_focus"):
        if marker not in main_text:
            raise SystemExit(f"src/main.c missing web sizing/focus boundary: {marker}")
    if "solar_web_report_state" not in main_text or "reportState" not in js_text:
        raise SystemExit("WebAssembly must report live simulation state to the Astro page")
    for marker in ("_solar_web_command", "addBody", "distanceM", "speedMps", "massKg", "radiusM"):
        if marker not in js_text:
            raise SystemExit(f"WebAssembly missing inspection/control bridge: {marker}")

    if "const int screen_width = 1280" in main_text or "const int screen_height = 720" in main_text:
        raise SystemExit("src/main.c must not hardcode the web InitWindow() size to 1280x720")

    print(f"WASM artifacts OK in {web_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
