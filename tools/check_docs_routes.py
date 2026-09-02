#!/usr/bin/env python3
"""Smoke-check generated Astro documentation routes.

The checker is intentionally boring: it asserts the Pages site contains the
common footer footprint, the dedicated /docs/ section, links to the WASM demo,
and the source-backed markers that should not disappear during visual edits.
"""

from __future__ import annotations

import sys
import os
from html.parser import HTMLParser
from pathlib import PurePosixPath
from pathlib import Path
from urllib.parse import unquote, urlsplit


BASE_PATH = "/solar-system-simulator/"


ROUTES: dict[str, list[str]] = {
    "index.html": [
        "Discover the solar system in motion",
        "favicon.ico",
        "data-footer-credits",
        "Jonathan Peris",
        "Illustrated worlds, source-backed orbits, no fake capabilities",
        "wasm/solar-system-simulator.html",
        "Dedicated WASM cockpit",
        "Source references",
        "Precise, playful, unfinished in public",
    ],
    "physics/index.html": ["Physics stays in SI units", "docs/simulation-core/", "data-footer-credits"],
    "body-catalog/index.html": ["Stable IDs prevent duplicate knowledge", "docs/roadmap/", "Phobos", "Deimos", "Vesta", "JPL SBDB solution 36"],
    "source-atlas/index.html": ["The code separates physics from presentation", "docs/architecture/", "src/sim/"],
    "pipeline/index.html": ["Native tests feed a Pages lab bench", "docs/build-and-web/", "make web"],
    "docs/index.html": ["Trace every orbit wire without one giant scroll", "Solar manual routes", "Every orbit manual page", "docs/architecture/"],
    "docs/architecture/index.html": ["Architecture keeps physics testable", "src/sim/", "src/render/", "src/main.c", "tests/"],
    "docs/simulation-core/index.html": ["Simulation state uses physical units first", "src/sim/physics.c", "src/sim/solar_system.c", "src/sim/vec3d.c"],
    "docs/rendering/index.html": ["Rendering adapts physics for human eyes", "src/render/renderer.c", "src/app/body_trails.c"],
    "docs/controls/index.html": ["Controls expose the current physics scene", "Tab", "V", "Mouse wheel", "orbit_camera"],
    "docs/build-and-web/index.html": ["Native checks feed the public WebAssembly lab", "make test", "make web", "make dist-wasm", ".github/workflows/build.yml", ".github/workflows/deploy-pages.yml"],
    "docs/roadmap/index.html": ["Expansion stays one body at a time", "Implemented now", "Planned sequence", "Jupiter", "Kuiper belt"],
}

FOOTER_MARKERS = ["data-footer-credits", "Jonathan Peris", "raylib", "Emscripten", "Astro", "GitHub Pages"]


class ReferenceParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.references: list[str] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        for name, value in attrs:
            if name in {"href", "src"} and value:
                self.references.append(value)


def fail(message: str) -> None:
    print(f"docs route check failed: {message}", file=sys.stderr)
    raise SystemExit(1)


def read_route(dist: Path, route: str) -> str:
    path = dist / route
    if not path.is_file():
        fail(f"missing generated route {route}")
    return path.read_text(encoding="utf-8", errors="replace")


def internal_target(route: str, reference: str) -> Path | None:
    parsed = urlsplit(reference)
    if parsed.scheme or parsed.netloc or not parsed.path:
        return None

    path = unquote(parsed.path)
    if path.startswith("/"):
        if not path.startswith(BASE_PATH):
            fail(f"{route} has root-relative reference outside {BASE_PATH}: {reference}")
        relative = path.removeprefix(BASE_PATH)
    else:
        relative = str(PurePosixPath(route).parent / path)

    target = PurePosixPath(relative)
    if path.endswith("/") or target.name == "":
        target /= "index.html"
    return Path(target)


def check_internal_references(dist: Path, route: str, html: str) -> None:
    parser = ReferenceParser()
    parser.feed(html)
    for reference in parser.references:
        target = internal_target(route, reference)
        if target is not None and not (dist / target).is_file():
            fail(f"{route} references missing local file: {reference}")


def main(argv: list[str]) -> int:
    dist = Path(argv[1]) if len(argv) > 1 else Path("docs/dist")
    if not dist.is_dir():
        fail(f"dist directory not found: {dist}")

    favicon = dist / "favicon.ico"
    if not favicon.is_file():
        fail("missing favicon.ico")
    if not favicon.read_bytes().startswith(b"\x00\x00\x01\x00"):
        fail("favicon.ico is not a valid ICO file")
    for asset in ("robots.txt", "sitemap.xml"):
        if not (dist / asset).is_file():
            fail(f"missing {asset}")

    analytics_id = os.environ.get("PUBLIC_GA_ID", "")

    for route, markers in ROUTES.items():
        html = read_route(dist, route)
        for marker in markers:
            if marker not in html:
                fail(f"{route} missing marker: {marker}")
        check_internal_references(dist, route, html)
        if "rel=\"canonical\"" not in html:
            fail(f"{route} missing canonical URL")
        if "Skip to content" not in html:
            fail(f"{route} missing keyboard skip link")
        if route == "index.html" and "role=\"table\"" in html:
            fail("index.html uses invalid presentational table roles")
        if analytics_id and analytics_id not in html:
            fail(f"{route} missing configured analytics ID")
        if not analytics_id and "googletagmanager.com/gtag/js" in html:
            fail(f"{route} includes analytics without PUBLIC_GA_ID")
        if route in {"index.html", "docs/index.html", "docs/build-and-web/index.html"}:
            for marker in FOOTER_MARKERS:
                if marker not in html:
                    fail(f"{route} missing footer marker: {marker}")
    wasm = dist / "wasm" / "solar-system-simulator.html"
    if not wasm.is_file():
        fail("missing copied WebAssembly HTML artifact")

    wasm_markers = [
        "Orbit cockpit runtime",
        "Launch-ready C/raylib canvas",
        "Static renderer notes now shown on the page",
        "Renderer behavior",
        "Controls expose real simulator state",
    ]
    wasm_html = wasm.read_text(encoding="utf-8", errors="replace")
    for marker in wasm_markers:
        if marker not in wasm_html:
            fail(f"copied WebAssembly HTML artifact missing marker: {marker}")
    check_internal_references(dist, "wasm/solar-system-simulator.html", wasm_html)

    print(f"Docs routes OK in {dist}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
