"""The planetary epoch snapshot must record each planet's own Horizons body."""
import copy
import importlib.util
import json
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('planet_epoch', ROOT / 'tools/planet_epoch.py')
planet_epoch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(planet_epoch)


class PlanetEpochProvenance(unittest.TestCase):
    def setUp(self):
        self.snapshot = json.loads(planet_epoch.DATA.read_text())

    def output_for(self, snapshot):
        with patch.object(planet_epoch.Path, 'read_text', return_value=json.dumps(snapshot)):
            return planet_epoch.output()

    def test_committed_snapshot_generates(self):
        self.assertIn('{', self.output_for(self.snapshot))

    def test_target_must_name_the_entrys_own_body(self):
        broken = copy.deepcopy(self.snapshot)
        earth = next(p for p in broken['planets'] if p['name'] == 'Earth')
        earth['target'] = 'Jupiter Barycenter (5)'
        with self.assertRaisesRegex(ValueError, 'not Horizons body 3'):
            self.output_for(broken)

    def test_planet_centre_is_rejected_for_moon_systems(self):
        broken = copy.deepcopy(self.snapshot)
        earth = next(p for p in broken['planets'] if p['name'] == 'Earth')
        earth['target'] = 'Earth (3)'
        with self.assertRaisesRegex(ValueError, 'not a system barycenter'):
            self.output_for(broken)


if __name__ == '__main__':
    unittest.main()
