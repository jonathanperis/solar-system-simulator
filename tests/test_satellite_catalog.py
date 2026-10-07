"""Satellite snapshot validation must hold even when Python strips asserts."""
import copy
import importlib.util
import json
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / 'tools/satellite_catalog.py'
spec = importlib.util.spec_from_file_location('satellites', TOOL)
satellites = importlib.util.module_from_spec(spec)
spec.loader.exec_module(satellites)


def snapshot(system):
    return json.loads(satellites.paths(system)[0].read_text())


class SatelliteValidation(unittest.TestCase):
    def test_committed_snapshots_are_valid_and_generated(self):
        for system in satellites.SYSTEMS:
            data = snapshot(system)
            satellites.validate(system, data)
            self.assertEqual(satellites.paths(system)[1].read_text(), satellites.generate(system, data), system)

    def test_out_of_range_elements_are_rejected(self):
        broken = copy.deepcopy(snapshot('jupiter'))
        broken['moons'][5]['eccentricity'] = 0.9
        with self.assertRaisesRegex(ValueError, 'orbital elements'):
            satellites.validate('jupiter', broken)

    def test_major_moon_set_is_explicit(self):
        broken = copy.deepcopy(snapshot('saturn'))
        hyperion = next(m for m in broken['moons'] if m['code'] == 607)
        hyperion['major'] = True
        with self.assertRaisesRegex(ValueError, 'major moons changed'):
            satellites.validate('saturn', broken)

    def test_uranus_inventory_keeps_one_puck_from_the_inner_moon_solution(self):
        moons = snapshot('uranus')['moons']
        pucks = [m for m in moons if m['code'] == 715]
        self.assertEqual([(m['ephemeris'], m['frame']) for m in pucks], [('URA184', 'Laplace')])
        equatorial = [m for m in moons if m['frame'] == 'equatorial']
        self.assertEqual({m['code'] for m in equatorial}, {701, 702, 703, 704, 705})
        # The equatorial plane's pole is Uranus's spin pole: the IAU pole's antipode.
        for m in equatorial:
            self.assertAlmostEqual(m['pole_ra_deg'], 77.311)
            self.assertAlmostEqual(m['pole_dec_deg'], 15.175)

    def test_duplicate_rows_need_a_preference(self):
        row = ['1', 'Neptune', 'Naiad', '803', 'NEP097', 'Laplace', '2000-01-01.5', '48200.', '0', '0', '0', '0', '0',
               '0.29', '0', '0', '299.7', '42.7', '0.4', '1']
        with self.assertRaisesRegex(ValueError, 'explicit ephemeris preference'):
            satellites.refresh('neptune', [row, row[:4] + ['NEP104'] + row[5:]], [])

    def test_provisional_designations_use_iau_form(self):
        self.assertEqual(satellites.iau_name('S2003_J_2'), 'S/2003 J 2')
        self.assertEqual(satellites.iau_name('S2023_U1'), 'S/2023 U 1')
        self.assertEqual(satellites.iau_name('S2025_U_1'), 'S/2025 U 1')
        self.assertEqual(satellites.iau_name('Triton'), 'Triton')

    def test_slugs_are_unique_across_systems(self):
        slugs = [m['slug'] for system in satellites.SYSTEMS for m in snapshot(system)['moons']]
        self.assertEqual(len(slugs), len(set(slugs)))

    def test_rejection_survives_optimized_python(self):
        # `python3 -O` removes assert statements; validation must not use them.
        script = (
            "import importlib.util, json, sys\n"
            f"spec = importlib.util.spec_from_file_location('s', {str(TOOL)!r})\n"
            "s = importlib.util.module_from_spec(spec); spec.loader.exec_module(s)\n"
            "d = json.load(open(s.paths('neptune')[0]))\n"
            "d['moons'].pop()\n"
            "try:\n    s.validate('neptune', d)\nexcept ValueError:\n    sys.exit(0)\n"
            "sys.exit(1)\n")
        result = subprocess.run([sys.executable, '-O', '-c', script], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == '__main__':
    unittest.main()
