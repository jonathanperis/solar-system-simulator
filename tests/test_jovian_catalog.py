"""Jovian snapshot validation must hold even when Python strips asserts."""
import copy
import importlib.util
import json
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('jovian', ROOT / 'tools/jovian_catalog.py')
jovian = importlib.util.module_from_spec(spec)
spec.loader.exec_module(jovian)


class JovianValidation(unittest.TestCase):
    def setUp(self):
        self.snapshot = json.loads((ROOT / 'data/jovian_moons.json').read_text())

    def test_committed_snapshot_is_valid(self):
        jovian.validate(self.snapshot)

    def test_out_of_range_elements_are_rejected(self):
        broken = copy.deepcopy(self.snapshot)
        broken['moons'][5]['eccentricity'] = 0.9
        with self.assertRaisesRegex(ValueError, 'orbital elements'):
            jovian.validate(broken)

    def test_rejection_survives_optimized_python(self):
        # `python3 -O` removes assert statements; validation must not use them.
        script = (
            "import importlib.util, json, sys\n"
            f"spec = importlib.util.spec_from_file_location('j', {str(ROOT / 'tools/jovian_catalog.py')!r})\n"
            "j = importlib.util.module_from_spec(spec); spec.loader.exec_module(j)\n"
            f"s = json.load(open({str(ROOT / 'data/jovian_moons.json')!r}))\n"
            "s['moons'].pop()\n"
            "try:\n    j.validate(s)\nexcept ValueError:\n    sys.exit(0)\n"
            "sys.exit(1)\n")
        result = subprocess.run([sys.executable, '-O', '-c', script], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == '__main__':
    unittest.main()
