#!/usr/bin/env python3
"""Smoke-check generated Astro documentation routes.

The checker is intentionally boring: it asserts the Pages site contains the
common footer footprint, the dedicated /docs/ section, links to the WASM demo,
and the source-backed markers that should not disappear during visual edits.
"""

from __future__ import annotations

import json
import os
import re
import sys
from html.parser import HTMLParser
from pathlib import PurePosixPath
from pathlib import Path
from typing import NoReturn
from urllib.parse import unquote, urlsplit
from xml.etree import ElementTree


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
    "compare/index.html": ["One question. Two experiments", "data-comparison", "learning-lab.mjs", "Guided challenges", "Force-contribution inspector", "data-config-error"],
    "simulator/index.html": ["Explore the solar system", "data-simulator", "runtime-control-state", "canvas", "15-second", "uniform", "data-footer-credits", "data-runtime-panel", "data-runtime-body", "data-runtime-speed", "Show this system", "Center on this object", "physical measurements", "data-inspector-distance", "data-inspector-speed", "data-runtime-search", "data-runtime-group", "15 days / second", "10 days / second", "data-runtime-achieved", "test particles", "data-body-filter-status"],
    "body-catalog/index.html": ["Find your next world", "data-body-search", "data-body-family", "docs/roadmap/", "Phobos", "Deimos", "Vesta", "Jupiter", "Saturn", "JPL physical parameters", "NASA's Saturn facts", "JPL SBDB solution 36"],
    "small-bodies/index.html": ["A million worlds", "data-small-atlas", "data-density", "data-catalog-search", "data-results", "data-basket", "data-prepare", "catalog/manifest.json", "1,564,244"],
    "source-atlas/index.html": ["The code separates physics from presentation", "docs/architecture/", "src/sim/"],
    "pipeline/index.html": ["Native tests feed a Pages lab bench", "docs/build-and-web/", "make web"],
    "docs/index.html": ["Find your way around the solar system", "Solar manual routes", "Every orbit manual page", "docs/architecture/", "Your first orbit"],
    "docs/architecture/index.html": ["Architecture keeps physics testable", "src/sim/", "src/render/", "src/main.c", "tests/"],
    "docs/experiments/index.html": ["Predict, run, measure, explain", "make headless", "solar-lab", "Barycentric Earth", "normalized_energy_change"],
    "docs/simulation-core/index.html": ["Simulation state uses physical units first", "src/sim/physics.c", "src/sim/solar_system.c", "src/sim/vec3d.c"],
    "docs/rendering/index.html": ["Rendering adapts physics for human eyes", "src/render/renderer.c", "src/app/body_trails.c"],
    "docs/controls/index.html": ["Make the view your own", "Space", "N", "R", "Frame", "physical inspector", "simulation_session", "orbit_camera", "Everyday controls"],
    "docs/build-and-web/index.html": ["Native checks feed the public WebAssembly lab", "make test", "make web", "make dist-wasm", ".github/workflows/build.yml", ".github/workflows/deploy-pages.yml"],
    "docs/roadmap/index.html": ["Expansion stays one body at a time", "Implemented now", "Planned sequence", "Jupiter", "Saturn", "Kuiper belt"],
}

FOOTER_MARKERS = ["data-footer-credits", "Jonathan Peris", "Learning laboratory", "raylib", "Emscripten", "Astro", "GitHub Pages"]
ATLAS_BODY_ANCHORS = ["sun", "mercury", "venus", "earth", "moon", "mars", "phobos", "deimos", "vesta", "jupiter"]
ATLAS_BODY_ANCHORS += [moon["slug"] for moon in json.loads((Path(__file__).resolve().parents[1] / "data/jovian_moons.json").read_text())["moons"]]
ATLAS_BODY_ANCHORS += ["saturn", "uranus", "neptune"]


class ReferenceParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__()
        self.references: list[str] = []
        self.elements: list[tuple[str, dict[str, str | None]]] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        self.elements.append((tag, dict(attrs)))
        for name, value in attrs:
            if name in {"href", "src", "data-runtime-src", "data-lab-src"} and value:
                self.references.append(value)


def fail(message: str) -> NoReturn:
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
        if target is not None:
            resolved = (dist / target).resolve()
            if not resolved.is_relative_to(dist.resolve()):
                fail(f"{route} references a file outside the published tree: {reference}")
            if not resolved.is_file():
                fail(f"{route} references missing local file: {reference}")


ASTRO_GENERATOR = re.compile(r'<meta\s+name="generator"\s+content="Astro v[0-9][^"]*"\s*/?>', re.IGNORECASE)
# Files that Astro sources own. A copy in docs/public would be published
# verbatim and bypass the shared layout, CSP and route inventory.
PUBLIC_FORBIDDEN_NAMES = {"sitemap.xml", "robots.txt"}


# Document types a browser can render as a page that runs script (SVG can
# embed <script>; XHTML/SHTML are HTML variants). None are published today;
# any future one must be named in ALLOWED_ACTIVE_DOCUMENTS after review.
ACTIVE_DOCUMENT_SUFFIXES = {".svg", ".svgz", ".xhtml", ".xht", ".shtml"}
ALLOWED_ACTIVE_DOCUMENTS: frozenset[str] = frozenset()


def check_astro_generated(dist: Path, allowed: frozenset[str] | set[str] = ALLOWED_ACTIVE_DOCUMENTS) -> None:
    """Every published HTML document must be rendered by an Astro page.

    BaseLayout emits `<meta name="generator" content={Astro.generator}>`, so a
    document without that exact head marker was hand-written or copied from
    docs/public, even if it mentions Astro elsewhere. Other scriptable document
    types are rejected unless explicitly allow-listed by relative path.
    """
    for page in sorted(dist.rglob("*")):
        if not page.is_file():
            continue
        route = page.relative_to(dist).as_posix()
        suffix = page.suffix.lower()
        if suffix in ACTIVE_DOCUMENT_SUFFIXES and route not in allowed:
            fail(f"{route} is a scriptable document type outside Astro; allow-list it explicitly after review")
        if suffix in {".html", ".htm"}:
            html = page.read_text(encoding="utf-8", errors="replace")
            if not ASTRO_GENERATOR.search(html):
                fail(f"{route} lacks the Astro generator marker; every page must come from Astro")


def check_not_found_page(dist: Path) -> None:
    """Astro must emit 404.html: GitHub Pages serves it at any missing URL, so
    it is noindex, has no canonical URL and links home with base-absolute paths."""
    page = dist / "404.html"
    if not page.is_file():
        fail("missing 404.html; render src/pages/404.astro")
    html = page.read_text(encoding="utf-8", errors="replace")
    if not ASTRO_GENERATOR.search(html):
        fail("404.html lacks the Astro generator marker")
    if not re.search(r'<meta\s+name="robots"\s+content="noindex"', html):
        fail("404.html must be noindex")
    if 'rel="canonical"' in html:
        fail("404.html must not declare a canonical URL")
    for target in (BASE_PATH, f"{BASE_PATH}simulator/"):
        if f'href="{target}"' not in html:
            fail(f"404.html must link to {target} with a base-absolute URL")


class SecurityParser(HTMLParser):
    """Collects head order and inline code that a strict CSP would block."""

    def __init__(self) -> None:
        super().__init__()
        self.head: list[tuple[str, dict[str, str | None]]] = []
        self.in_head = False
        self.inline: list[str] = []

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        values = dict(attrs)
        if tag == "head":
            self.in_head = True
        elif self.in_head:
            self.head.append((tag, values))
        if "style" in values:
            self.inline.append(f"style attribute on <{tag}>")
        if tag == "style":
            self.inline.append("<style> element")
        if tag == "script" and not values.get("src"):
            self.inline.append("inline <script>")
        if any(name.startswith("on") for name in values):
            self.inline.append(f"inline event handler on <{tag}>")

    def handle_endtag(self, tag: str) -> None:
        if tag == "head":
            self.in_head = False


ANALYTICS_WILDCARDS = {
    "connect-src": {"https://*.google-analytics.com", "https://*.analytics.google.com", "https://*.googletagmanager.com"},
    "img-src": {"https://*.google-analytics.com", "https://*.googletagmanager.com"},
}


def check_page_security(route: str, html: str) -> None:
    """Pages deliver a strict meta CSP before any subresource and contain no
    inline code that would require 'unsafe-inline' or hashes (A59)."""
    parser = SecurityParser()
    parser.feed(html)
    resources = [(tag, attrs) for tag, attrs in parser.head if not (tag == "meta" and "charset" in attrs)]
    if not resources or resources[0][0] != "meta" or \
            (resources[0][1].get("http-equiv") or "").lower() != "content-security-policy":
        fail(f"{route} must start <head> with its Content-Security-Policy meta tag")
    policy = resources[0][1].get("content") or ""
    directives = {part.split()[0]: part.split()[1:] for part in policy.split(";") if part.split()}
    for name in ("default-src", "script-src", "style-src", "object-src"):
        if name not in directives:
            fail(f"{route} CSP lacks {name}")
    if any(token in {"'unsafe-inline'", "'unsafe-eval'"} for sources in directives.values() for token in sources):
        fail(f"{route} CSP must not allow unsafe-inline or unsafe-eval")
    # Wildcards are rejected everywhere except Google's documented GA4
    # collection subdomains, which rotate by region and so cannot be listed
    # exactly. They are allowed only in the two fetch directives analytics uses
    # and only appear in builds that carry PUBLIC_GA_ID (V17).
    for name, sources in directives.items():
        for token in sources:
            if "*" in token and token not in ANALYTICS_WILDCARDS.get(name, ()):
                fail(f"{route} CSP {name} must not allow wildcard source {token}")
    if parser.inline:
        fail(f"{route} contains {parser.inline[0]}; move it to a file so the CSP stays strict")


def check_public_sources(public: Path, allowed: frozenset[str] | set[str] = ALLOWED_ACTIVE_DOCUMENTS) -> None:
    """docs/public holds static assets only; HTML and the sitemap come from Astro."""
    for asset in sorted(public.rglob("*")):
        if not asset.is_file():
            continue
        name = asset.relative_to(public).as_posix()
        if asset.suffix.lower() in ACTIVE_DOCUMENT_SUFFIXES and name not in allowed:
            fail(f"docs/public/{name} is a scriptable document type; allow-list it explicitly after review")
        if asset.suffix.lower() in {".html", ".htm"}:
            fail(f"docs/public/{name} is hand-written HTML; render it from an Astro page instead")
        if name.lower() in PUBLIC_FORBIDDEN_NAMES:
            fail(f"docs/public/{name} must be generated by Astro, not copied verbatim")


def check_sitemap(dist: Path) -> None:
    # Derive the canonical page inventory from output, so a new route cannot be
    # silently omitted from both the hand-maintained sitemap and ROUTES above.
    expected = {
        "https://jonathanperis.github.io" + BASE_PATH +
        page.relative_to(dist).as_posix().removesuffix("index.html")
        for page in dist.rglob("index.html")
    }
    try:
        sitemap = ElementTree.parse(dist / "sitemap.xml")
    except ElementTree.ParseError as error:
        fail(f"invalid sitemap.xml: {error}")
    locations = [node.text for node in sitemap.findall(
        "{http://www.sitemaps.org/schemas/sitemap/0.9}url/"
        "{http://www.sitemaps.org/schemas/sitemap/0.9}loc")]
    if len(locations) != len(expected) or set(locations) != expected:
        fail("sitemap.xml must list each generated canonical page exactly once")


def main(argv: list[str]) -> int:
    dist = Path(argv[1]) if len(argv) > 1 else Path("docs/dist")
    if not dist.is_dir():
        fail(f"dist directory not found: {dist}")

    favicon = dist / "favicon.ico"
    if not favicon.is_file():
        fail("missing favicon.ico")
    if not favicon.read_bytes().startswith(b"\x00\x00\x01\x00"):
        fail("favicon.ico is not a valid ICO file")
    if not (dist / "sitemap.xml").is_file():
        fail("missing sitemap.xml")
    check_sitemap(dist)
    check_astro_generated(dist)
    check_not_found_page(dist)
    for page in sorted(dist.rglob("*.html")):
        check_page_security(page.relative_to(dist).as_posix(), page.read_text(encoding="utf-8", errors="replace"))
    check_public_sources(Path(__file__).resolve().parents[1] / "docs" / "public")

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
        primary_paths = {BASE_PATH + path for path in ("simulator/", "docs/experiments/", "compare/", "docs/")}
        if len(primary_links) != 4 or {attrs.get("href") for attrs in primary_links} != primary_paths:
            fail(f"{route} must expose Explore, Learn, Experiments and Reference navigation destinations")
        if sum(attrs.get("aria-current") in {"page", "location"} for attrs in primary_links) != 1:
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
