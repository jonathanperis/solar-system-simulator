"""Exercise the public CLI and parse its physical, streaming CSV output."""
import csv
import io
import math
import os
import resource
import signal
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
    def test_csv_permissions_preserve_new_and_existing_file_contracts(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as directory:
            for index, (mask, existing, expected) in enumerate(((0, None, 0o600), (0o777, None, 0o600), (0, 0o640, 0o640))):
                with self.subTest(umask=mask, existing=existing):
                    output = Path(directory) / f"series-{index}.csv"
                    if existing is not None:
                        output.write_text("old output")
                        output.chmod(existing)
                    result = subprocess.run([str(RUNNER), "--duration", "15", "--sample", "15", "--output", str(output)],
                                            capture_output=True, text=True, umask=mask)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(stat.S_IMODE(output.stat().st_mode), expected)
                    self.assertTrue(output.read_text().startswith("# solar-lab-v1"))

    @unittest.skipUnless(os.name == "posix", "POSIX symlink semantics")
    def test_csv_output_refuses_symlinks_without_touching_the_target(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as directory:
            target = Path(directory) / "precious.txt"
            target.write_text("keep me")
            for name, link_to in (("link.csv", target), ("dangling.csv", Path(directory) / "missing.txt")):
                with self.subTest(link=name):
                    link = Path(directory) / name
                    link.symlink_to(link_to)
                    result = self.run_lab("--duration", "15", "--sample", "15", "--output", str(link))
                    self.assertNotEqual(result.returncode, 0)
                    self.assertTrue(link.is_symlink())
                    self.assertEqual(target.read_text(), "keep me")
                    self.assertFalse((Path(directory) / "missing.txt").exists())
            self.assertEqual(sorted(p.name for p in Path(directory).iterdir()), ["dangling.csv", "link.csv", "precious.txt"])

    @unittest.skipUnless(hasattr(os, "mkfifo"), "POSIX FIFO semantics")
    def test_csv_output_refuses_fifos_without_blocking(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as directory:
            fifo = Path(directory) / "pipe.csv"
            os.mkfifo(fifo)
            result = subprocess.run([str(RUNNER), "--duration", "15", "--sample", "15", "--output", str(fifo)],
                                    capture_output=True, text=True, timeout=10)
            self.assertNotEqual(result.returncode, 0)
            self.assertTrue(stat.S_ISFIFO(fifo.stat().st_mode))
            self.assertEqual([p.name for p in Path(directory).iterdir()], ["pipe.csv"])

    @unittest.skipUnless(os.name == "posix", "POSIX file-size limit")
    def test_failed_run_leaves_no_partial_csv(self):
        def limit_file_size():
            # Writes past 4 KiB fail with EFBIG instead of killing the process,
            # simulating a full disk midway through a long series.
            signal.signal(signal.SIGXFSZ, signal.SIG_IGN)
            resource.setrlimit(resource.RLIMIT_FSIZE, (4096, 4096))

        with tempfile.TemporaryDirectory(dir=ROOT / "build") as directory:
            existing = Path(directory) / "existing.csv"
            existing.write_text("previous complete output")
            existing.chmod(0o640)
            fresh = Path(directory) / "fresh.csv"
            for output in (existing, fresh):
                with self.subTest(output=output.name):
                    result = subprocess.run([str(RUNNER), "--duration", "864000", "--sample", "15", "--output", str(output)],
                                            capture_output=True, text=True, preexec_fn=limit_file_size, timeout=60)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn("CSV", result.stderr)
            self.assertEqual(existing.read_text(), "previous complete output")
            self.assertEqual(stat.S_IMODE(existing.stat().st_mode), 0o640)
            self.assertFalse(fresh.exists())
            self.assertEqual([p.name for p in Path(directory).iterdir()], ["existing.csv"])

    @unittest.skipUnless(os.name == "posix", "POSIX file permission contract")
    def test_successful_run_replaces_existing_output_completely(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "build") as directory:
            output = Path(directory) / "series.csv"
            output.write_text("x" * 100000)
            result = self.run_lab("--duration", "15", "--sample", "15", "--output", str(output))
            self.assertEqual(result.returncode, 0, result.stderr)
            text = output.read_text()
            self.assertTrue(text.startswith("# solar-lab-v1"))
            self.assertNotIn("x" * 10, text)
            self.assertEqual([p.name for p in Path(directory).iterdir()], ["series.csv"])


if __name__ == "__main__":
    unittest.main()
