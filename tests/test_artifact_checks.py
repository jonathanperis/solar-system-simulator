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
from check_docs_routes import check_internal_references
from check_wasm_artifacts import main as check_wasm


class ArtifactChecks(unittest.TestCase):
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
