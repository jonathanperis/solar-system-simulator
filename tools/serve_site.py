#!/usr/bin/env python3
"""Serve built Pages on loopback; explicit fixtures exercise real loader failures."""
import argparse
import re
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit

parser = argparse.ArgumentParser()
parser.add_argument("directory", type=Path)
parser.add_argument("--port", type=int, default=4318)
parser.add_argument("--test-fixtures", action="store_true")
args = parser.parse_args()
root = args.directory.resolve()
base = "/solar-system-simulator/"


class Handler(SimpleHTTPRequestHandler):
    def translate_path(self, path):
        path = urlsplit(path).path
        if path.startswith(base):
            path = path[len(base):]
        target = (root / path.lstrip("/")).resolve()
        return str(target) if target.is_relative_to(root) else str(root / "missing")

    def send_bytes(self, data, content_type):
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        path = urlsplit(self.path).path
        if args.test_fixtures and path.startswith("/__test__/"):
            fixtures = {
                "/__test__/missing-runtime/": ("simulator/index.html", "data-runtime-src", "/missing-runtime.js"),
                "/__test__/bad-runtime/": ("simulator/index.html", "data-runtime-src", "/__test__/bad-runtime/solar-system-simulator.js"),
                "/__test__/bad-lab/": ("compare/index.html", "data-lab-src", "/__test__/bad-lab/learning-lab.mjs"),
            }
            if path in fixtures:
                page, attribute, url = fixtures[path]
                html = re.sub(fr'{attribute}="[^"]+"', f'{attribute}="{url}"', (root / page).read_text())
                return self.send_bytes(html.encode(), "text/html; charset=utf-8")
            if path in ("/__test__/bad-runtime/solar-system-simulator.js", "/__test__/bad-lab/learning-lab.mjs"):
                return self.send_bytes((root / "wasm" / Path(path).name).read_bytes(), "text/javascript")
            if path in ("/__test__/bad-runtime/solar-system-simulator.wasm", "/__test__/bad-lab/learning-lab.wasm"):
                return self.send_bytes(b"invalid local test fixture", "application/wasm")
        return super().do_GET()


print(f"Serving {base} at http://127.0.0.1:{args.port}", flush=True)
ThreadingHTTPServer(("127.0.0.1", args.port), Handler).serve_forever()
