#!/usr/bin/env python3
"""Smoke-check generated Astro documentation routes.

The checker is intentionally boring: it asserts the Pages site contains the
common footer footprint, the dedicated /docs/ section, links to the WASM demo,
and the source-backed markers that should not disappear during visual edits.
"""

from __future__ import annotations

import sys
import os
import json
from html.parser import HTMLParser
from pathlib import PurePosixPath
from pathlib import Path
from urllib.parse import unquote, urlsplit


BASE_PATH = "/solar-system-simulator/"


ROUTES: dict[str, list[str]] = {
    "index.html": [
        "A solar system, drawn for inspection",
        "favicon.ico",
        "data-footer-credits",
        "Jonathan Peris",
        "data-orbital-atlas",
        "Heliocentric",
        "Earth system",
        "Mars system",
        "Jupiter system",
        "data-atlas-moon-group",
        "simulator/",
        "Run live simulator",
    ],
    "physics/index.html": ["Physics stays in SI units", "docs/simulation-core/", "data-footer-credits"],
    "simulator/index.html": ["Run the real orbit loop", "data-simulator", "runtime-control-state", "canvas", "15-second", "uniform", "data-footer-credits", "data-runtime-panel", "data-runtime-body", "data-runtime-speed", "Frame selected system", "Frame selected body", "Live physics inspector", "data-inspector-distance", "data-inspector-speed", "data-runtime-search", "data-runtime-group", "15 days / second", "10 days / second", "data-runtime-achieved", "test particles"],
    "body-catalog/index.html": ["Stable IDs prevent duplicate knowledge", "docs/roadmap/", "Phobos", "Deimos", "Vesta", "Jupiter", "Saturn", "JPL physical parameters", "NASA's Saturn facts", "JPL SBDB solution 36"],
    "source-atlas/index.html": ["The code separates physics from presentation", "docs/architecture/", "src/sim/"],
    "pipeline/index.html": ["Native tests feed a Pages lab bench", "docs/build-and-web/", "make web"],
    "docs/index.html": ["Trace every orbit wire without one giant scroll", "Solar manual routes", "Every orbit manual page", "docs/architecture/"],
    "docs/architecture/index.html": ["Architecture keeps physics testable", "src/sim/", "src/render/", "src/main.c", "tests/"],
    "docs/simulation-core/index.html": ["Simulation state uses physical units first", "src/sim/physics.c", "src/sim/solar_system.c", "src/sim/vec3d.c"],
    "docs/rendering/index.html": ["Rendering adapts physics for human eyes", "src/render/renderer.c", "src/app/body_trails.c"],
    "docs/controls/index.html": ["Controls expose the current physics scene", "Space", "N", "R", "Frame", "physical inspector", "simulation_session", "orbit_camera"],
    "docs/build-and-web/index.html": ["Native checks feed the public WebAssembly lab", "make test", "make web", "make dist-wasm", ".github/workflows/build.yml", ".github/workflows/deploy-pages.yml"],
    "docs/roadmap/index.html": ["Expansion stays one body at a time", "Implemented now", "Planned sequence", "Jupiter", "Saturn", "Kuiper belt"],
}

FOOTER_MARKERS = ["data-footer-credits", "Jonathan Peris", "Milestone 11", "raylib", "Emscripten", "Astro", "GitHub Pages"]
ATLAS_BODY_ANCHORS = ["sun", "mercury", "venus", "earth", "moon", "mars", "phobos", "deimos", "vesta", "jupiter"]
ATLAS_BODY_ANCHORS += [moon["slug"] for moon in json.loads((Path(__file__).resolve().parents[1] / "data/jovian_moons.json").read_text())["moons"]]
ATLAS_BODY_ANCHORS += ["saturn"]


class ReferenceParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.references: list[str] = []
        self.elements: list[tuple[str, dict[str, str | None]]] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        self.elements.append((tag, dict(attrs)))
        for name, value in attrs:
            if name in {"href", "src", "data-runtime-src"} and value:
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
        parser = ReferenceParser()
        parser.feed(html)
        primary_links = [
            attrs
            for tag, attrs in parser.elements
            if tag == "a" and "data-primary-nav-link" in attrs
        ]
        if len(primary_links) != 7:
            fail(f"{route} must expose all seven primary navigation links")
        if sum(attrs.get("aria-current") == "page" for attrs in primary_links) != 1:
            fail(f"{route} must identify exactly one current primary navigation link")
        if "rel=\"canonical\"" not in html:
            fail(f"{route} missing canonical URL")
        if "Skip to content" not in html:
            fail(f"{route} missing keyboard skip link")
        if route == "index.html" and "role=\"table\"" in html:
            fail("index.html uses invalid presentational table roles")
        if route == "index.html":
            body_controls = [
                (tag, attrs)
                for tag, attrs in parser.elements
                if "data-atlas-body" in attrs
            ]
            if len(body_controls) != len(ATLAS_BODY_ANCHORS):
                fail("index.html must expose one control for each atlas body")
            if any(tag != "button" or "aria-pressed" not in attrs for tag, attrs in body_controls):
                fail("index.html atlas bodies must be semantic toggle buttons")
            for slug in ATLAS_BODY_ANCHORS:
                if f"body-catalog/#{slug}" not in html:
                    fail(f"index.html missing no-JS body catalog fallback: {slug}")
        if analytics_id and analytics_id not in html:
            fail(f"{route} missing configured analytics ID")
        if not analytics_id and "googletagmanager.com" in html:
            fail(f"{route} contacts Google Tag Manager without PUBLIC_GA_ID")
        if route in {"index.html", "docs/index.html", "docs/build-and-web/index.html"}:
            for marker in FOOTER_MARKERS:
                if marker not in html:
                    fail(f"{route} missing footer marker: {marker}")
    for extension in ("js", "wasm"):
        if not (dist / "wasm" / f"solar-system-simulator.{extension}").is_file():
            fail(f"missing WebAssembly runtime asset: {extension}")
    redirect = read_route(dist, "wasm/solar-system-simulator.html")
    if "http-equiv=\"refresh\"" not in redirect or f"{BASE_PATH}simulator/" not in redirect:
        fail("legacy WebAssembly HTML URL must redirect to the Astro simulator")

    print(f"Docs routes OK in {dist}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
