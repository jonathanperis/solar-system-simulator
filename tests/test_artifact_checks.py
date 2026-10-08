"""Negative contract tests for structural bundle and internal-route validation."""
import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from check_docs_routes import check_astro_generated, check_internal_references, check_not_found_page, check_page_security, check_public_sources, check_sitemap
from check_wasm_artifacts import main as check_wasm


class ArtifactChecks(unittest.TestCase):
    def test_every_generated_html_document_carries_the_astro_marker(self):
        astro = '<html><head><meta name="generator" content="Astro v7.3.6"></head></html>'
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as directory:
            root = Path(directory)
            (root / "docs").mkdir()
            (root / "index.html").write_text(astro)
            (root / "docs/index.html").write_text(astro)
            (root / "legacy.html").write_text(astro)
            check_astro_generated(root)
            for name, html in (("docs/index.html", "<html><head></head></html>"),
                               ("stray.htm", "<html>hand written</html>"),
                               ("legacy.html", '<meta name="generator" content="Hugo">'),
                               ("docs/index.html", "<!-- Astro v7 --><html></html>")):
                with self.subTest(name=name, html=html), self.assertRaises(SystemExit):
                    (root / name).write_text(html)
                    check_astro_generated(root)
                (root / name).write_text(astro)

    def test_pages_carry_a_leading_strict_csp_without_inline_code(self):
        csp = ('<meta http-equiv="Content-Security-Policy" content="default-src \'self\'; '
               'script-src \'self\' \'wasm-unsafe-eval\'; style-src \'self\'; object-src \'none\'">')
        head = f'<html><head><meta charset="utf-8">{csp}<link rel="stylesheet" href="a.css">'
        check_page_security("index.html", head + '<script type="module" src="a.js"></script></head><body></body></html>')
        # Google's documented GA4 subdomain wildcards are the only permitted
        # wildcards, and only in the fetch directives analytics needs (V17).
        analytics = head.replace("object-src 'none'", "object-src 'none'; connect-src 'self' https://*.google-analytics.com "
                                 "https://*.analytics.google.com https://*.googletagmanager.com; "
                                 "img-src 'self' https://*.google-analytics.com https://*.googletagmanager.com")
        check_page_security("index.html", analytics + '</head></html>')
        for html in (
            '<html><head><meta charset="utf-8"><script src="a.js"></script></head></html>',
            '<html><head><link rel="stylesheet" href="a.css">' + csp + '</head></html>',
            head + '<script>alert(1)</script></head></html>',
            head + '</head><body><style>p{}</style></body></html>',
            head + '</head><body><p style="color:red">x</p></body></html>',
            head.replace("style-src 'self'", "style-src 'self' 'unsafe-inline'") + '</head></html>',
            head.replace("script-src 'self'", "script-src 'self' 'unsafe-eval'") + '</head></html>',
            head.replace("object-src 'none'", "object-src 'none'; img-src https://*.example.com") + '</head></html>',
            head.replace("script-src 'self'", "script-src 'self' https://*.google-analytics.com") + '</head></html>',
            head.replace("object-src 'none'", "object-src 'none'; connect-src *") + '</head></html>',
        ):
            with self.subTest(html=html), self.assertRaises(SystemExit):
                check_page_security("index.html", html)

    def test_scriptable_document_types_need_an_explicit_allow_list(self):
        astro = '<html><head><meta name="generator" content="Astro v7.3.6"></head></html>'
        for name in ("icon.svg", "logo.SVGZ", "page.xhtml", "page.xht", "include.shtml"):
            with tempfile.TemporaryDirectory(dir=ROOT / "build") as directory:
                dist = Path(directory) / "dist"
                public = Path(directory) / "public"
                dist.mkdir()
                public.mkdir()
                (dist / "index.html").write_text(astro)
                check_astro_generated(dist)
                check_public_sources(public)
                with self.subTest(where="dist", name=name), self.assertRaises(SystemExit):
                    (dist / name).write_text('<svg xmlns="http://www.w3.org/2000/svg"><script>alert(1)</script></svg>')
                    check_astro_generated(dist)
                with self.subTest(where="public", name=name), self.assertRaises(SystemExit):
                    (public / name).write_text("<svg/>")
                    check_public_sources(public)
                with self.subTest(where="allow-list", name=name):
                    check_astro_generated(dist, allowed={name})
                    check_public_sources(public, allowed={name})

    def test_not_found_page_is_an_astro_noindex_page_linking_home(self):
        good = ('<html><head><meta name="generator" content="Astro v7.3.6"><meta name="robots" content="noindex">'
                '</head><body><a href="/solar-system-simulator/">Simulator</a>'
                '<a href="/solar-system-simulator/learn/">Learn</a><a href="/solar-system-simulator/catalog/">Catalog</a>'
                '<a href="/solar-system-simulator/about/">About</a></body></html>')
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as directory:
            dist = Path(directory)
            with self.assertRaises(SystemExit):
                check_not_found_page(dist)
            (dist / "404.html").write_text(good)
            check_not_found_page(dist)
            for html in (good.replace('<meta name="generator" content="Astro v7.3.6">', ""),
                         good.replace('<meta name="robots" content="noindex">', ""),
                         good.replace('href="/solar-system-simulator/learn/"', 'href="learn/"'),
                         good.replace('href="/solar-system-simulator/"', 'href="../"'),
                         good.replace("</head>", '<link rel="canonical" href="https://example.test/"></head>')):
                with self.subTest(html=html), self.assertRaises(SystemExit):
                    (dist / "404.html").write_text(html)
                    check_not_found_page(dist)

    def test_public_assets_cannot_contain_hand_written_html_or_robots(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as directory:
            public = Path(directory)
            (public / "wasm").mkdir()
            (public / "wasm/runtime.js").write_text("// generated runtime")
            (public / "favicon.ico").write_bytes(b"\x00\x00\x01\x00")
            check_public_sources(public)
            for name in ("wasm/solar-system-simulator.html", "notes.HTM", "sitemap.xml", "robots.txt"):
                with self.subTest(name=name), self.assertRaises(SystemExit):
                    (public / name).write_text("<html></html>")
                    try:
                        check_public_sources(public)
                    finally:
                        (public / name).unlink()

    def test_sitemap_matches_generated_pages(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as directory:
            root = Path(directory)
            (root / "index.html").write_text("home")
            (root / "small-bodies").mkdir()
            (root / "small-bodies/index.html").write_text("catalog")
            # A retired URL's redirect stub is noindex and not a sitemap page.
            (root / "physics").mkdir()
            (root / "physics/index.html").write_text('<meta name="robots" content="noindex">')
            base = "https://jonathanperis.github.io/solar-system-simulator/"
            expected = [base, base + "small-bodies/"]

            def sitemap(urls):
                (root / "sitemap.xml").write_text(
                    '<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">' +
                    "".join(f"<url><loc>{url}</loc></url>" for url in urls) + "</urlset>")

            sitemap(expected)
            check_sitemap(root)
            for urls in ([base], expected + [base + "removed/"], expected + [base]):
                with self.subTest(urls=urls), self.assertRaises(SystemExit):
                    sitemap(urls)
                    check_sitemap(root)

    def test_missing_assets_and_base_path_escape_are_rejected(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as directory:
            root = Path(directory) / "dist"
            root.mkdir()
            (root.parent / "outside.js").write_text("// outside published tree")
            (root / "asset.js").write_text("// fixture")
            check_internal_references(root, "index.html", '<script src="/solar-system-simulator/asset.js"></script>')
            for reference in ("/asset.js", "/solar-system-simulator/missing.js", "/solar-system-simulator/../outside.js"):
                with self.subTest(reference=reference), self.assertRaises(SystemExit):
                    check_internal_references(root, "index.html", f'<script src="{reference}"></script>')

    def test_bundle_provenance_detects_changed_or_missing_companions(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as directory:
            root = Path(directory)
            names = ["solar-system-simulator.js", "solar-system-simulator.wasm", "catalog-orbits.wasm", "learning-lab.mjs", "learning-lab.wasm"]
            markers = "solar-system-simulator.wasm reportState _solar_web_command _solar_web_experiment _solar_web_demo " \
                "_solar_web_lesson _solar_web_export reportLabState downloadCsv clearBodies ccall addBody distanceM speedMps " \
                "massKg radiusM massQuality radiusQuality achievedTimeScale pendingSeconds"
            (root / names[0]).write_text(markers)
            for name in (names[1], names[2], names[4]):
                (root / name).write_bytes(b"\x00asm\x01\x00\x00\x00")
            (root / names[3]).write_text('learning-lab.wasm _lab_start _lab_advance _lab_point _lab_export_csv')
            manifest = {"schema": 1, "revision": "test-fixture", "files": {
                name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in names}}
            (root / "build-info.json").write_text(json.dumps(manifest))
            with patch.object(sys, "argv", ["checker", str(root)]):
                self.assertEqual(check_wasm(), 0)
                # A custom section is id 0, then its byte size, name length and name.
                def custom(name, payload=b"x"):
                    body = bytes([len(name)]) + name + payload
                    return b"\x00" + bytes([len(body)]) + body
                for name, debug in ((names[1], True), (names[2], True), (names[4], True), (names[1], False)):
                    section = custom(b".debug_info" if debug else b"producers")
                    with self.subTest(module=name, debug=debug):
                        (root / name).write_bytes(b"\x00asm\x01\x00\x00\x00" + section)
                        manifest["files"][name] = hashlib.sha256((root / name).read_bytes()).hexdigest()
                        (root / "build-info.json").write_text(json.dumps(manifest))
                        if debug:
                            with self.assertRaisesRegex(SystemExit, "debug"):
                                check_wasm()
                        else:
                            self.assertEqual(check_wasm(), 0)
                        (root / name).write_bytes(b"\x00asm\x01\x00\x00\x00")
                        manifest["files"][name] = hashlib.sha256((root / name).read_bytes()).hexdigest()
                        (root / "build-info.json").write_text(json.dumps(manifest))
                (root / names[1]).write_bytes(b"\x00asm\x01\x00\x00\x00" + b"\x00\x05")
                manifest["files"][names[1]] = hashlib.sha256((root / names[1]).read_bytes()).hexdigest()
                (root / "build-info.json").write_text(json.dumps(manifest))
                with self.assertRaisesRegex(SystemExit, "truncated"):
                    check_wasm()
                # A custom-section name may not run past its own section, even
                # when later bytes in the file would make the slice succeed.
                (root / names[1]).write_bytes(b"\x00asm\x01\x00\x00\x00" + b"\x00\x02\x03x" + b"\x01\x00")
                manifest["files"][names[1]] = hashlib.sha256((root / names[1]).read_bytes()).hexdigest()
                (root / "build-info.json").write_text(json.dumps(manifest))
                with self.assertRaisesRegex(SystemExit, "name runs past"):
                    check_wasm()
                (root / names[1]).write_bytes(b"not wasm")
                with self.assertRaisesRegex(SystemExit, "checksum mismatch"):
                    check_wasm()
                manifest["files"][names[1]] = hashlib.sha256(b"not wasm").hexdigest()
                (root / "build-info.json").write_text(json.dumps(manifest))
                with self.assertRaisesRegex(SystemExit, "magic"):
                    check_wasm()
                (root / names[1]).unlink()
                with self.assertRaisesRegex(SystemExit, "missing required artifact"):
                    check_wasm()


if __name__ == "__main__":
    unittest.main()
