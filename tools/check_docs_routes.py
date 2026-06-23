#!/usr/bin/env python3
"""Smoke-check generated Astro documentation routes.

The checker is intentionally boring: it asserts the Pages site contains the
common footer footprint, the dedicated /docs/ section, links to the WASM demo,
and the source-backed markers that should not disappear during visual edits.
"""

from __future__ import annotations

import sys
from pathlib import Path


ROUTES: dict[str, list[str]] = {
    "index.html": [
        "Gravity, but make it a weird science poster",
        "data-footer-credits",
        "Jonathan Peris",
        "Young nerd energy, source-backed science, zero stock-space fog",
        "wasm/solar-system-simulator.html",
        "Source references",
        "Readable, playable, unfinished on purpose",
    ],
    "physics/index.html": ["Physics stays in SI units", "docs/simulation-core/", "data-footer-credits"],
    "body-catalog/index.html": ["Stable IDs prevent duplicate knowledge", "docs/roadmap/", "Phobos", "Deimos"],
    "source-atlas/index.html": ["The code separates physics from presentation", "docs/architecture/", "src/sim/"],
    "pipeline/index.html": ["Native tests feed a Pages lab bench", "docs/build-and-web/", "make web"],
    "docs/index.html": ["Astro docs for the C solar-system lab", "Code explainer index", "docs/architecture/"],
    "docs/architecture/index.html": ["Architecture keeps physics testable", "src/sim/", "src/render/", "src/main.c", "tests/"],
    "docs/simulation-core/index.html": ["Simulation state uses physical units first", "src/sim/physics.c", "src/sim/solar_system.c", "src/sim/vec3d.c"],
    "docs/rendering/index.html": ["Rendering adapts physics for human eyes", "src/render/renderer.c", "src/app/body_labels.c", "src/app/body_trails.c"],
    "docs/controls/index.html": ["Controls expose the current physics scene", "Tab", "V", "Mouse wheel", "orbit_camera"],
    "docs/build-and-web/index.html": ["Native checks feed the public WebAssembly lab", "make test", "make web", "make dist-wasm", ".github/workflows/build.yml", ".github/workflows/deploy-pages.yml"],
    "docs/roadmap/index.html": ["Expansion stays one body at a time", "Implemented now", "Planned sequence", "Jupiter", "Kuiper belt"],
}

FOOTER_MARKERS = ["data-footer-credits", "Jonathan Peris", "raylib", "Emscripten", "Astro", "GitHub Pages"]


def fail(message: str) -> None:
    print(f"docs route check failed: {message}", file=sys.stderr)
    raise SystemExit(1)


def read_route(dist: Path, route: str) -> str:
    path = dist / route
    if not path.is_file():
        fail(f"missing generated route {route}")
    return path.read_text(encoding="utf-8", errors="replace")


def main(argv: list[str]) -> int:
    dist = Path(argv[1]) if len(argv) > 1 else Path("docs/dist")
    if not dist.is_dir():
        fail(f"dist directory not found: {dist}")

    for route, markers in ROUTES.items():
        html = read_route(dist, route)
        for marker in markers:
            if marker not in html:
                fail(f"{route} missing marker: {marker}")
        if route in {"index.html", "docs/index.html", "docs/build-and-web/index.html"}:
            for marker in FOOTER_MARKERS:
                if marker not in html:
                    fail(f"{route} missing footer marker: {marker}")

    wasm = dist / "wasm" / "solar-system-simulator.html"
    if not wasm.is_file():
        fail("missing copied WebAssembly HTML artifact")

    print(f"Docs routes OK in {dist}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
