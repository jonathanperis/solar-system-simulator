"""Exercise the public CLI and parse its physical, streaming CSV output."""
import csv
import io
import math
import os
import stat
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / os.environ.get("SOLAR_LAB_BINARY", "build/solar-lab")


class HeadlessLab(unittest.TestCase):
    def run_lab(self, *args):
        return subprocess.run([str(RUNNER), *args], capture_output=True, text=True)

    def test_replayable_samples_and_integrator_comparison(self):
        args = ["--scene", "circular", "--duration", "86400", "--dt", "300", "--sample", "43200"]
        a = self.run_lab(*args)
        b = self.run_lab(*args)
        self.assertEqual(a.returncode, 0, a.stderr)
        self.assertEqual(a.stdout, b.stdout)
        self.assertIn("# scene: circular", a.stdout)
        self.assertIn("# dt_seconds: 300", a.stdout)
        self.assertIn("# revision:", a.stdout)
        rows = list(csv.DictReader(io.StringIO("\n".join(line for line in a.stdout.splitlines() if not line.startswith("#")))))
        self.assertEqual(len(rows), 6)
        earth = rows[-1]
        self.assertEqual(float(earth["time_s"]), 86400)
        self.assertEqual(int(earth["tick"]), 288)
        self.assertEqual(earth["name"], "Earth")
        radius = math.hypot(float(earth["x_m"]), float(earth["z_m"]))
        self.assertLess(abs(radius / 149597870700 - 1), 1e-6)
        euler = self.run_lab(*args, "--integrator", "euler")
        self.assertEqual(euler.returncode, 0, euler.stderr)
        self.assertNotEqual(euler.stdout, a.stdout)

    def test_invalid_or_unaligned_inputs_fail_without_csv(self):
        for args in [("--dt", "nan"), ("--duration", "31"), ("--sample", "14"),
                     ("--scene", "typo"), ("--velocity-factor", "0"), ("--dt", "0"),
                     ("--integrator", "rk4"), ("--scene", "core", "--integrator", "euler")]:
            with self.subTest(args=args):
                result = self.run_lab(*args)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, "")
                self.assertTrue(result.stderr)

    def test_catalog_csv_escapes_names_and_keeps_unknown_values_explicit(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as directory:
            path = Path(directory) / "experiment.txt"
            path.write_text('SOLAR_EXPERIMENT_V1 2461200.5\n1000000\tA, "quoted"\t1\t0\t0\t0\t0\t2461200.5\t0\t0\t2\t2\n')
            result = self.run_lab("--experiment", str(path), "--duration", "15", "--sample", "15")
            self.assertEqual(result.returncode, 0, result.stderr)
            rows = list(csv.DictReader(io.StringIO("\n".join(line for line in result.stdout.splitlines() if not line.startswith("#")))))
            self.assertEqual(rows[-1]["name"], 'A, "quoted"')
            self.assertEqual(rows[-1]["mass_kg"], "")
            self.assertEqual(rows[-1]["mass_quality"], "unknown")

    def test_comparison_descriptor_replays_both_series(self):
        result = self.run_lab("--compare", str(ROOT / "examples/collision.solar"))
        self.assertEqual(result.returncode, 0, result.stderr)
        rows = list(csv.DictReader(io.StringIO("\n".join(line for line in result.stdout.splitlines() if not line.startswith("#")))))
        self.assertEqual(len(rows), 21)
        self.assertEqual(rows[-1]["body_count_a"], "2")
        self.assertEqual(rows[-1]["body_count_b"], "1")
        self.assertEqual(rows[-1]["position_difference_m"], "")
        self.assertEqual(rows[-1]["subject_present_b"], "0")

    @unittest.skipUnless(os.name == "posix", "POSIX file permission contract")
    def test_new_csv_output_is_private_even_with_a_permissive_umask(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as directory:
            output = Path(directory) / "series.csv"
            result = subprocess.run([str(RUNNER), "--duration", "15", "--sample", "15", "--output", str(output)],
                                    capture_output=True, text=True, umask=0)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(output.read_text().startswith("# solar-lab-v1"))
            self.assertEqual(stat.S_IMODE(output.stat().st_mode), 0o600)


if __name__ == "__main__":
    unittest.main()
