import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import gzip
import json

spec = importlib.util.spec_from_file_location('catalog', Path(__file__).parents[1] / 'tools/small_body_catalog.py')
catalog = importlib.util.module_from_spec(spec)
spec.loader.exec_module(catalog)


class CatalogTest(unittest.TestCase):
    def test_snapshot_accounts_for_missing_orbits_and_rejects_duplicates(self):
        fields = catalog.FIELDS
        def row(id, kind='an', orbit='MBA', q=2):
            values = dict(spkid=id, pdes=str(id), full_name='Example', kind=kind,
                          **{'class': orbit}, epoch=2461200.5, equinox='J2000',
                          q=q, e=.1, i=3, om=4, w=5, tp=2461000.5,
                          GM=None, diameter=10, condition_code='2')
            return [values[f] for f in fields]
        payload = {'signature': {'version': '1.0'}, 'fields': fields, 'count': 3,
                   'data': [row(20000001), row(20000002, q=None), row(10000001, 'cn', 'JFC')]}
        records, excluded = catalog.normalize(payload)
        self.assertEqual((len(records), excluded), (2, 1))
        self.assertTrue(catalog.usable(records[0]))
        self.assertFalse(catalog.usable(records[1]))
        self.assertIsNone(records[0][11])
        payload['data'][1][0] = 20000001
        with self.assertRaises(ValueError):
            catalog.normalize(payload)

    def test_regeneration_audits_lookup_ranges_before_replacing_snapshot(self):
        record=[20000004,'4','Vesta','an','MBA',2461200.5,'J2000',2.148,.09,7,103,151,2461000.5,None,None,'0']
        payload={'signature':{'version':'1.0'},'fields':catalog.FIELDS,'count':1,'data':[record]}
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory); output=root/'catalog'; cache=root/'source.json.gz'
            cache.write_bytes(gzip.compress(json.dumps(payload).encode()))
            with patch.object(catalog,'OUT',output), patch.object(catalog,'CACHE',cache):
                # Use explicit path because the CLI's default argument is fixed
                # when the module is imported.
                original_check=catalog.check
                with patch.object(catalog,'check',side_effect=lambda path=None,allow_previous_schema=False:original_check(path or output,allow_previous_schema)):
                    catalog.generate()
                    manifest_path=output/'manifest.json'
                    manifest=json.loads(manifest_path.read_text())
                    manifest['schema']=1
                    for shard in manifest['shards']:
                        shard['index'].pop('contentSha256')
                        shard['data'].pop('contentSha256')
                    manifest['overview'].pop('contentSha256')
                    manifest_path.write_text(json.dumps(manifest))
                    catalog.generate()
                    self.assertFalse(output.with_name('catalog-previous').exists())
                    manifest=json.loads(manifest_path.read_text())
                    manifest['shards'][0]['minId']=1
                    (output/'manifest.json').write_text(json.dumps(manifest))
                    with self.assertRaisesRegex(ValueError,'lookup range'):
                        original_check(output)
        payload['signature']['version'] = 'unexpected'
        with self.assertRaises(ValueError):
            catalog.normalize(payload)


if __name__ == '__main__':
    unittest.main()
