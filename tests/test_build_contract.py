"""Exercise Make's exit-status and transitive-header contracts without editing sources."""
import os
import re
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class BuildContract(unittest.TestCase):
    def make(self, *args, env=None):
        return subprocess.run(["make", "--no-print-directory", *args], cwd=ROOT,
                              capture_output=True, text=True, env=env)

    def test_environment_flags_extend_but_never_replace_required_flags(self):
        # Packagers commonly export CFLAGS/CPPFLAGS. They may tune optimization
        # but must not drop the project include path, C11 mode, warnings or the
        # no-FMA-contraction rule that keeps native and WASM results identical.
        env = {**os.environ, "CFLAGS": "-O3", "CPPFLAGS": "-I/opt/example/include"}
        dry_run = self.make("-n", "-B", "build/tests/test_vec3d", env=env)
        self.assertEqual(dry_run.returncode, 0, dry_run.stderr)
        for flag in ("-Isrc", "-std=c11", "-Wall", "-ffp-contract=off", "-O3", "-I/opt/example/include"):
            self.assertIn(flag, dry_run.stdout)

    def test_web_builds_use_required_flags_without_native_debug_info(self):
        # Shipped WebAssembly must not inherit the native `-O2 -g` default:
        # DWARF sections quadruple the download and embed absolute build paths.
        # Every emcc compile still carries the C11/warning/no-FMA contract and
        # -O3 (SPEC A97: worth 4 fps in the throttled Saturn scene).
        env = {**os.environ, "CFLAGS": "-O0 -g"}
        targets = ["build/web/solar-system-simulator.js", "build/web/learning-lab.mjs", "build/web/catalog-orbits.wasm"]
        # A stand-in raylib checkout whose Makefile does nothing keeps the dry
        # run independent of a real Emscripten raylib build.
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as raylib:
            (Path(raylib) / "Makefile").write_text("all:\n\t@true\n")
            dry_run = self.make("-n", "-B", *targets, f"RAYLIB_WEB_SRC={raylib}", env=env)
        self.assertEqual(dry_run.returncode, 0, dry_run.stderr)
        emcc = [line for line in dry_run.stdout.splitlines() if line.startswith("emcc ")]
        self.assertEqual(len(emcc), 3, dry_run.stdout)
        for line in emcc:
            with self.subTest(line=line[:80]):
                flags = line.split()
                for flag in ("-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-ffp-contract=off", "-O3", "-Isrc"):
                    self.assertIn(flag, flags)
                self.assertNotIn("-g", flags)
                self.assertNotIn("-O0", flags)

    def test_recursive_make_does_not_duplicate_required_flags(self):
        dry_run = self.make("-n", "-B", "test-sanitize")
        self.assertEqual(dry_run.returncode, 0, dry_run.stderr)
        compiles = [line for line in dry_run.stdout.splitlines() if "-fsanitize=" in line and " -o " in line]
        self.assertTrue(compiles, dry_run.stdout)
        for line in compiles:
            with self.subTest(line=line[:80]):
                self.assertEqual(line.split().count("-std=c11"), 1)
                self.assertEqual(line.split().count("-Isrc"), 1)

    def test_tests_refuse_to_compile_without_assert(self):
        # The C tests use assert() for every check; -DNDEBUG would silently turn
        # them into passing no-ops, so each test must fail to compile instead.
        for source in sorted((ROOT / "tests").glob("test_*.c")):
            with self.subTest(source=source.name):
                result = subprocess.run(
                    ["cc", "-std=c11", "-Isrc", "-Ibuild", "-DNDEBUG", "-fsyntax-only", str(source)],
                    cwd=ROOT, capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("NDEBUG", result.stderr)

    def test_first_failure_reaches_make(self):
        result = self.make("test-binaries", "TEST_BINS=/usr/bin/false /usr/bin/true")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Running /usr/bin/false", result.stdout)
        self.assertNotIn("Running /usr/bin/true", result.stdout)

    def test_transitive_vector_header_rebuilds_test_and_orbit_library(self):
        targets = ["build/tests/test_vec3d", "build/catalog-orbits.dylib"]
        built = self.make(*targets)
        self.assertEqual(built.returncode, 0, built.stderr)
        self.assertEqual(self.make("-q", *targets).returncode, 0)
        changed = self.make("-n", "-W", "src/sim/vec3d.h", *targets)
        self.assertEqual(changed.returncode, 0, changed.stderr)
        self.assertIn("-o build/tests/test_vec3d", changed.stdout)
        self.assertIn("-o build/catalog-orbits.dylib", changed.stdout)

    def test_simulation_layer_has_no_raylib_or_presentation_policy(self):
        # V1 plus A58: src/sim owns SI physics only. Render units, illustrative
        # radii and visual ring ratios belong to src/render/render_scale.*.
        # Match raylib/render *includes*, not prose: a comment saying
        # "raylib-independent" is allowed, any spelling of the header is not.
        forbidden = re.compile(r"(?i:#\s*include\s*[<\"][^>\"]*(?:raylib|render/)[^>\"]*[>\"])|\bVector3\b|"
                               r"SOLAR_RENDER_\w+|SOLAR_ILLUSTRATIVE_\w+|"
                               r"SOLAR_MIN_VISIBLE_\w+|\w*_VISUAL_\w*|meters_(?:vec_)?to_render\w*")
        for source in sorted((ROOT / "src/sim").rglob("*")):
            if source.suffix not in {".c", ".h", ".inc"}:
                continue
            with self.subTest(source=source.name):
                self.assertEqual(forbidden.findall(source.read_text()), [])
        # The pattern itself must accept prose and reject real includes.
        self.assertIsNone(forbidden.search("/* raylib-independent physics */"))
        for line in ('#include "raylib.h"', "#include <Raylib.h>", '#  include "render/render_scale.h"'):
            self.assertIsNotNone(forbidden.search(line), line)


if __name__ == "__main__":
    unittest.main()
