#!/usr/bin/env python3
"""Explicit, single-response SBDB harvest. Normal builds verify pinned shards.

Full precision orbital records stay outside the JS bundle and C scene. Search
indexes are class-partitioned and scan one bounded shard at a time. The overview
counts every usable source orbit through the same C kernel used by the runtime.
"""
import argparse
from collections import Counter, defaultdict
import ctypes
from datetime import datetime, timezone
import gzip
import hashlib
import json
import math
from pathlib import Path
import shutil
import tempfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'docs/public/catalog'
CACHE = ROOT / 'build/catalog-source.json.gz'
FIELDS = ['spkid', 'pdes', 'full_name', 'kind', 'class', 'epoch', 'equinox',
          'q', 'e', 'i', 'om', 'w', 'tp', 'GM', 'diameter', 'condition_code']
RECORD_FIELDS = ['id', 'name', 'designation', 'class', 'epoch', 'q', 'e', 'i',
                 'node', 'periapsis', 'tp', 'gm', 'diameter', 'condition']
URL = 'https://ssd-api.jpl.nasa.gov/sbdb_query.api?fields=' + ','.join(FIELDS) + '&full-prec=true'
EPOCH = 2461200.5
SHARD_ROWS = 8192
CLASS_NAMES = {'MBA': 'Main belt', 'IMB': 'Inner belt', 'OMB': 'Outer belt',
               'TNO': 'Trans-Neptunian', 'CEN': 'Centaurs', 'TJN': 'Jupiter Trojans',
               'IEO': 'Atira', 'ATE': 'Aten', 'APO': 'Apollo', 'AMO': 'Amor',
               'MCA': 'Mars crossing', 'AST': 'Other asteroids', 'HYA': 'Hyperbolic',
               'PAA': 'Parabolic'}


def usable(r):
    return (all(isinstance(r[i], (float, int)) and math.isfinite(r[i]) for i in range(4, 11))
            and r[5] > 0 and r[6] >= 0 and 0 <= r[7] <= 180)


def normalize(payload):
    if payload.get('signature', {}).get('version') != '1.0' or payload.get('fields') != FIELDS:
        raise ValueError('SBDB schema changed: review before refreshing')
    rows = payload.get('data', [])
    if len(rows) != int(payload['count']):
        raise ValueError('incomplete bulk source response')
    seen, records, excluded = set(), [], 0
    for row in rows:
        if len(row) != len(FIELDS):
            raise ValueError('source row width changed')
        d = dict(zip(FIELDS, row))
        id = int(d['spkid'])
        if id in seen or not 0 < id < 2147483647:
            raise ValueError(f'duplicate or unsupported SPK identity: {id}')
        seen.add(id)
        if not (d['kind'].startswith('a') or d['class'] in ('CEN', 'TNO')):
            excluded += 1
            continue
        if d['equinox'] != 'J2000':
            raise ValueError(f'unsupported source frame: {id}')
        numeric = [float(d[f]) if d[f] is not None else None
                   for f in ('epoch', 'q', 'e', 'i', 'om', 'w', 'tp', 'GM', 'diameter')]
        if any(v is not None and not math.isfinite(v) for v in numeric):
            raise ValueError(f'nonfinite source data: {id}')
        if any(v is not None and v <= 0 for v in numeric[-2:]):
            raise ValueError(f'invalid physical parameter: {id}')
        records.append([id, d['full_name'].strip(), d['pdes'], d['class'], *numeric, d['condition_code']])
    return sorted(records, key=lambda r: (r[3], r[0])), excluded


def packed(path, data):
    raw = json.dumps(data, separators=(',', ':'), ensure_ascii=False, allow_nan=False).encode()
    content = gzip.compress(raw, compresslevel=9, mtime=0)
    path.write_bytes(content)
    return {'file': path.name, 'bytes': len(content),
            'sha256': hashlib.sha256(content).hexdigest(),
            'contentSha256': hashlib.sha256(raw).hexdigest()}


def harvest():
    CACHE.parent.mkdir(parents=True, exist_ok=True)
    temporary = CACHE.with_suffix('.partial')
    request = urllib.request.Request(URL, headers={'Accept-Encoding': 'gzip', 'User-Agent': 'SolarSystemSimulator/1.0 (educational offline catalog)'})
    print('Fetching one full-precision SBDB bulk response...', flush=True)
    with urllib.request.urlopen(request, timeout=600) as response, temporary.open('wb') as dest:
        if response.headers.get('Content-Encoding') == 'gzip':
            shutil.copyfileobj(response, dest)
        else:
            with gzip.GzipFile(fileobj=dest, mode='wb', mtime=0) as compressed:
                shutil.copyfileobj(response, compressed)
    # Partial transfers never become the accepted source snapshot.
    with gzip.open(temporary, 'rt') as handle:
        payload = json.load(handle)
    normalize(payload)
    temporary.rename(CACHE)
    print(f'Cached {payload["count"]} source rows in {CACHE.stat().st_size:,} bytes', flush=True)


def generate():
    with gzip.open(CACHE, 'rt') as handle:
        payload = json.load(handle)
    records, excluded = normalize(payload)
    source_count = int(payload['count'])
    del payload
    lib = ctypes.CDLL(str(ROOT / 'build/catalog-orbits.dylib'))
    coordinate = lib.catalog_coordinate
    coordinate.argtypes = [ctypes.c_double]*7 + [ctypes.c_int]
    coordinate.restype = ctypes.c_double
    staging = Path(tempfile.mkdtemp(prefix='.catalog-build-', dir=OUT.parent))
    counts = Counter(r[3] for r in records)
    shards, density = [], defaultdict(Counter)
    missing = mapped = 0
    # Logarithmic AU cells retain the distant tail instead of hiding outliers.
    # Coordinates are a density overview at a common epoch, not N-body motion.
    for start in range(0, len(records), SHARD_ROWS):
        batch = records[start:start+SHARD_ROWS]
        # Split at class boundaries to allow cheap outer-body-only searches.
        classes = sorted(set(r[3] for r in batch))
        for cls in classes:
            group = [r for r in batch if r[3] == cls]
            stem = f'{cls}-{start//SHARD_ROWS:04d}'
            detail = packed(staging / f'{stem}.json.gz', group)
            index = packed(staging / f'{stem}-index.json.gz',
                           [[r[0], r[1], r[2], r[5], r[12], usable(r)] for r in group])
            shards.append({'class': cls, 'count': len(group), 'minId': group[0][0], 'maxId': group[-1][0],
                           'index': index, 'data': detail})
            for r in group:
                if not usable(r):
                    missing += 1
                    continue
                x = coordinate(r[5], r[6], r[7], r[8], r[9], r[10], EPOCH, 0)/149597870700
                z = coordinate(r[5], r[6], r[7], r[8], r[9], r[10], EPOCH, 2)/149597870700
                if not math.isfinite(x) or not math.isfinite(z):
                    raise ValueError(f'C orbit conversion failed for {r[0]}')
                radius = math.hypot(x, z)
                log_radius = math.log10(1+radius)
                angle = math.atan2(z, x)
                cell = (round(log_radius*math.cos(angle)*60), round(log_radius*math.sin(angle)*60))
                density[cls][cell] += 1
                mapped += 1
        if start % (SHARD_ROWS*16) == 0:
            print(f'Packed/mapped {min(start+SHARD_ROWS,len(records)):,}/{len(records):,}', flush=True)
    overview = packed(staging / 'overview.json.gz',
                      {cls: [[x, y, n] for (x, y), n in sorted(cells.items())] for cls, cells in density.items()})
    manifest = {'schema': 2, 'checked': datetime.now(timezone.utc).isoformat(), 'source': URL,
                'sourceSignature': 'NASA/JPL SBDB Query API 1.0', 'sourceCount': source_count,
                'excludedOtherComets': excluded, 'count': len(records), 'mappable': mapped,
                'unavailableOrbits': missing, 'epoch': EPOCH, 'fields': RECORD_FIELDS,
                'classes': dict(counts), 'classNames': CLASS_NAMES, 'shards': shards, 'overview': overview,
                'sourceSha256': hashlib.sha256(CACHE.read_bytes()).hexdigest(),
                'model': 'Heliocentric two-body propagation to the reference epoch; J2000 ecliptic axes. Not ephemerides.',
                'physicalQuality': 'Published SBDB values; measurement/estimate classification not supplied by bulk API.'}
    (staging / 'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    check(staging)
    previous = OUT.with_name('catalog-previous')
    if previous.exists():
        raise ValueError('previous snapshot backup exists; inspect it before another refresh')
    if OUT.exists():
        check(allow_previous_schema=True) # Verify the shipped schema before replacing it.
        OUT.rename(previous)
    staging.rename(OUT)
    if previous.exists():
        shutil.rmtree(previous)
    check()


def check(directory=OUT, allow_previous_schema=False):
    m = json.loads((directory / 'manifest.json').read_text())
    if (m['schema'] != 2 and not (allow_previous_schema and m['schema'] == 1)) or m['fields'] != RECORD_FIELDS:
        raise ValueError('snapshot schema mismatch')
    count = missing = 0
    seen, classes = set(), Counter()
    expected_files = {'manifest.json'}
    for shard in m['shards']:
        for asset in (shard['index'], shard['data']):
            raw = (directory / asset['file']).read_bytes()
            expected_files.add(asset['file'])
            content = gzip.decompress(raw)
            if (len(raw) != asset['bytes'] or hashlib.sha256(raw).hexdigest() != asset['sha256']
                    or (m['schema'] == 2 and hashlib.sha256(content).hexdigest() != asset['contentSha256'])):
                raise ValueError(f'corrupt catalog asset: {asset["file"]}')
        records = json.loads(gzip.decompress((directory/shard['data']['file']).read_bytes()))
        index = json.loads(gzip.decompress((directory/shard['index']['file']).read_bytes()))
        if len(records) != shard['count'] or index != [[r[0],r[1],r[2],r[5],r[12],usable(r)] for r in records]:
            raise ValueError('index/data mismatch')
        ids=[r[0] for r in records]
        if not ids or ids!=sorted(ids) or shard['minId']!=ids[0] or shard['maxId']!=ids[-1]:
            raise ValueError('identity lookup range mismatch')
        if any(len(r[1].encode('utf-8'))>=96 or any(c in r[1] for c in '\t\r\n') for r in records):
            raise ValueError('source name exceeds native experiment contract')
        for r in records:
            if r[0] in seen or r[3] != shard['class']:
                raise ValueError('duplicate identity or class mismatch')
            seen.add(r[0]); classes[r[3]] += 1; count += 1
            missing += not usable(r)
    overview_asset = m['overview']
    raw = (directory/overview_asset['file']).read_bytes()
    expected_files.add(overview_asset['file'])
    if (len(raw) != overview_asset['bytes'] or hashlib.sha256(raw).hexdigest() != overview_asset['sha256']
            or (m['schema'] == 2 and hashlib.sha256(gzip.decompress(raw)).hexdigest() != overview_asset['contentSha256'])):
        raise ValueError('corrupt overview')
    overview = json.loads(gzip.decompress(raw))
    if sum(cell[2] for cells in overview.values() for cell in cells) != count-missing:
        raise ValueError('density overview lost records')
    if (count != m['count'] or missing != m['unavailableOrbits'] or count-missing != m['mappable']
            or classes != m['classes'] or count+m['excludedOtherComets'] != m['sourceCount']):
        raise ValueError('catalog accounting mismatch')
    if set(p.name for p in directory.iterdir()) != expected_files:
        raise ValueError('unexpected snapshot files')
    size = sum(p.stat().st_size for p in directory.iterdir())
    if size > 800_000_000:
        raise ValueError('catalog exceeds reserved Pages budget')
    print(f'Catalog verified: {count:,} entries; {missing} unavailable orbits; {size:,} bytes; {len(m["shards"])} shards')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--refresh', action='store_true')
    parser.add_argument('--generate', action='store_true')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    if args.refresh:
        harvest()
    if args.generate:
        generate()
    if args.check:
        check()
