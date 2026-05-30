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

    expected_html = [
        "Solar System Simulator WASM",
        "Solar System Simulator WebAssembly runtime",
        f"{stem}.js",
    ]
    for marker in expected_html:
        if marker not in html_text:
            raise SystemExit(f"{html} missing marker: {marker}")

    if f"{stem}.wasm" not in js_text:
        raise SystemExit(f"{js} does not reference {stem}.wasm")

    print(f"WASM artifacts OK in {web_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
