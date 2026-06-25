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
    html = web_dir / f"{stem}.html"
    js = web_dir / f"{stem}.js"
    wasm = web_dir / f"{stem}.wasm"

    for path in (html, js, wasm):
        require(path)

    html_text = html.read_text(encoding="utf-8")
    js_text = js.read_text(encoding="utf-8", errors="replace")
    repo_root = Path(__file__).resolve().parents[1]
    main_text = (repo_root / "src" / "main.c").read_text(encoding="utf-8")

    expected_html = [
        "Orbit cockpit runtime",
        "Solar System Simulator WebAssembly runtime",
        "favicon.ico",
        "Launch-ready C/raylib canvas",
        "Static renderer notes now shown on the page",
        "Renderer behavior",
        "Responsive frame",
        f"{stem}.js",
    ]
    for marker in expected_html:
        if marker not in html_text:
            raise SystemExit(f"{html} missing marker: {marker}")

    if f"{stem}.wasm" not in js_text:
        raise SystemExit(f"{js} does not reference {stem}.wasm")

    for marker in ("solar_web_initial_canvas_width", "solar_web_initial_canvas_height"):
        if marker not in main_text:
            raise SystemExit(f"src/main.c must use {marker} before InitWindow() so the WebGL window starts at the served frame size")

    if "const int screen_width = 1280" in main_text or "const int screen_height = 720" in main_text:
        raise SystemExit("src/main.c must not hardcode the web InitWindow() size to 1280x720")

    print(f"WASM artifacts OK in {web_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
