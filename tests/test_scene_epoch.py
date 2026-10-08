"""Scene epoch snapshot: identity, coverage and generation hold offline."""
import copy
import importlib.util
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('scene_epoch', ROOT / 'tools/scene_epoch.py')
scene_epoch = importlib.util.module_from_spec(spec)
spec.loader.exec_module(scene_epoch)


class SceneEpochSnapshot(unittest.TestCase):
    def setUp(self):
        self.snapshot = json.loads(scene_epoch.DATA.read_text())

    def test_committed_snapshot_is_valid_and_generated(self):
        scene_epoch.validate(self.snapshot)
        self.assertEqual(scene_epoch.GENERATED.read_text(), scene_epoch.generate(self.snapshot))

    def test_a_code_that_resolves_to_an_asteroid_is_not_a_moon(self):
        self.assertFalse(scene_epoch.identity_matches('75052 (1999 UM50)', 'S/2025 U 1', 75052))
        self.assertTrue(scene_epoch.identity_matches('S2023_S63 (65303)', 'S/2023 S 63', 65303))
        self.assertTrue(scene_epoch.identity_matches('Io (501)', 'Io', 501))
        self.assertTrue(scene_epoch.identity_matches('Pluto Barycenter (9)', 'Pluto Barycenter', 999))

    def test_every_catalogued_body_is_covered_once(self):
        broken = copy.deepcopy(self.snapshot)
        broken['bodies'].pop()
        with self.assertRaisesRegex(ValueError, 'cover every catalogued body'):
            scene_epoch.validate(broken)

    def test_undated_bodies_are_few_and_named(self):
        undated = self.snapshot['undated']
        self.assertLessEqual(len(undated), 5)
        for body in undated:
            self.assertTrue(body['name'] and body['code'])

    def test_states_are_in_kilometres(self):
        io = next(b for b in self.snapshot['bodies'] if b['code'] == 501)
        distance = sum(x * x for x in io['position_km']) ** 0.5
        self.assertTrue(400_000 < distance < 440_000)


if __name__ == '__main__':
    unittest.main()
