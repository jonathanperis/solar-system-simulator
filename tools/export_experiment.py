#!/usr/bin/env python3
"""Export up to 16 pinned SPK identities for native --experiment input."""
import argparse
import gzip
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def export(ids):
    if not 1 <= len(ids) <= 16 or len(set(ids)) != len(ids):
        raise ValueError('choose 1–16 distinct SPK identities')
    directory=ROOT/'docs/public/catalog'
    manifest=json.loads((directory/'manifest.json').read_text())
    supplement=json.loads((ROOT/'data/small_body_physical.json').read_text())['bodies']
    wanted=set(ids); found={}
    for shard in manifest['shards']:
        if not any(shard['minId'] <= id <= shard['maxId'] for id in wanted): continue
        with gzip.open(directory/shard['data']['file'],'rt') as handle:
            for row in json.load(handle):
                if row[0] in wanted: found[row[0]]=row;wanted.remove(row[0])
        if not wanted: break
    if wanted: raise ValueError(f'unknown identities: {sorted(wanted)}')
    lines=[f'SOLAR_EXPERIMENT_V1 {manifest["epoch"]}']
    for id in ids:
        r=found[id]
        if any(v is None for v in r[4:11]): raise ValueError(f'orbit unavailable: {id}')
        p=supplement.get(str(id))
        parameters=[p['massKg'],p['radiusM'],p['massQuality'],p['radiusQuality']] if p else [
            r[11]*1e9/6.67430e-11 if r[11] is not None else 0,
            r[12]*500 if r[12] is not None else 0,3 if r[11] is not None else 2,3 if r[12] is not None else 2]
        lines.append('\t'.join(map(str,[id,r[1],*r[5:11],*parameters])))
    return '\n'.join(lines)+'\n'

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('ids',nargs='+',type=int)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    args.output.write_text(export(args.ids))
    print(f'Exported {len(args.ids)} bodies to {args.output}')
