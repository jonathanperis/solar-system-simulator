"""Exercise Make's exit-status and transitive-header contracts without editing sources."""
import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class BuildContract(unittest.TestCase):
    def make(self, *args):
        return subprocess.run(["make", "--no-print-directory", *args], cwd=ROOT,
                              capture_output=True, text=True)

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


if __name__ == "__main__":
    unittest.main()
