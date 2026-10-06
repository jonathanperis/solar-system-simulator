import assert from 'node:assert/strict';
import test from 'node:test';
import { createHash } from 'node:crypto';
import { gzipSync } from 'node:zlib';
import { matchesIndex, experimentText, fetchPacked, physicalValues, numericIdentityCandidates, routeShards, ColumnarIndex, columnarRowLimit, integrityUnavailableMessage, catalogAssetUrl, parseWorkerMessage, createProgressAnnouncer } from '../src/lib/catalog.ts';

test('catalog filters retain provisional designations, missing size, and explicit physical supplements', () => {
  const row = [20134340,'134340 Pluto (1930 BM)','134340',29.6,null,true];
  assert(matchesIndex(row,'1930 bm',100,'known'));
  assert(!matchesIndex(row,'pluto',20,''));
  assert(!matchesIndex(row,'pluto',100,'unknown'));
  const record = [20134340,row[1],row[2],'TNO',2457588.5,29.6,.25,17,110,113,2447812,null,null,null];
  assert.equal(physicalValues(record).radiusM,1188300);
  const text = experimentText([record],2461200.5);
  assert.match(text,/^SOLAR_EXPERIMENT_V1 2461200.5\n20134340\t/);
  assert.throws(() => experimentText([record,record],2461200.5),/duplicate/i);
  assert.throws(() => experimentText(Array(17).fill(record),2461200.5),/16/);
  const missing = [...record]; missing[5]=null;
  assert.throws(() => experimentText([missing],2461200.5),/orbit/i);
});

test('digits-only queries are identity lookups routed by manifest ID ranges', () => {
  assert.deepEqual(numericIdentityCandidates(' 433 '), [20000433]);
  assert.deepEqual(numericIdentityCandidates('20134340'), [20134340]);
  assert.deepEqual(numericIdentityCandidates('54000000'), [54000000]);
  for (const text of ['', '1930 BM', '433 Eros', '12a', '0']) assert.equal(numericIdentityCandidates(text)?.length ? 1 : 0, 0, text);
  const manifest = { shards: [
    { class: 'AMO', minId: 20000433, maxId: 50986795 }, { class: 'MBA', minId: 20000001, maxId: 20000300 },
    { class: 'MBA', minId: 20000301, maxId: 20001000 }, { class: 'TNO', minId: 20015760, maxId: 54000000 }] };
  assert.deepEqual(routeShards(manifest, [20000433], '').map(s => s.class), ['AMO', 'MBA']);
  assert.deepEqual(routeShards(manifest, [20000433], 'MBA'), [manifest.shards[2]]);
  assert.deepEqual(routeShards(manifest, [10], ''), []);
});

test('the columnar index searches case-insensitively, filters, paginates and restores display names', () => {
  const index = new ColumnarIndex(6);
  index.append([[20000433,'433 Eros (A898 PA)','433',1.13,16.84,true], [20000719,'719 Albert (A911 TB)','719',1.19,null,true],
    [54000001,'(2020 AB)','2020 AB',null,null,false]], 'AMO');
  index.append([[20001934,'1934 Jeffers (1972 XB)','1934',2.1,null,true], [20002367,'2367 Praha (1981 AK)','2367',2.0,9,true],
    [20007796,'7796 Járacimrman (1996 BG)','7796',2.2,null,true]], 'MBA');
  index.finish();
  assert.equal(index.count, 6);
  const all = { query: '', qmax: Infinity, size: '', group: '' };
  assert.deepEqual(index.search({ ...all, query: 'EROS' }, 0, 50).hits, [{ id: 20000433, name: '433 Eros (A898 PA)', perihelion: 1.13, orbit: true, group: 'AMO' }]);
  assert.deepEqual(index.search({ ...all, query: 'járacim' }, 0, 50).hits.map(h => h.name), ['7796 Járacimrman (1996 BG)']);
  assert.deepEqual(index.search({ ...all, query: '19' }, 0, 50).hits.map(h => h.id), [20000719, 20001934, 20002367, 20007796]);
  assert.deepEqual(index.search({ ...all, query: '19', group: 'MBA' }, 0, 50).hits.map(h => h.id), [20001934, 20002367, 20007796]);
  assert.deepEqual(index.search({ ...all, qmax: 1.15 }, 0, 50).hits.map(h => h.id), [20000433]);
  assert.deepEqual(index.search({ ...all, size: 'known' }, 0, 50).hits.map(h => h.id), [20000433, 20002367]);
  assert.equal(index.search({ ...all, size: 'unknown' }, 0, 50).total, 4);
  const page = index.search({ ...all, query: '(' }, 1, 4);
  assert.equal(page.total, 6); assert.deepEqual(page.hits.map(h => h.id), [20002367, 20007796]);
  assert.deepEqual(index.search({ ...all, query: 'ignored' }, 0, 50, [20002367]).hits.map(h => h.name), ['2367 Praha (1981 AK)']);
  assert.equal(index.search({ ...all, query: 'pa)\n719' }, 0, 50).total, 0);
  assert.deepEqual(index.search(all, 0, 50).hits.find(h => h.id === 54000001), { id: 54000001, name: '(2020 AB)', perihelion: null, orbit: false, group: 'AMO' });
  assert.throws(() => new ColumnarIndex(columnarRowLimit + 1), /bound/);
  assert.throws(() => index.append([[1,'x','x',1,null,true]], 'MBA'), /more rows/);
});

test('download progress reaches the live region at most every 10% or 5 s', () => {
  let clock = 0;
  const announce = createProgressAnnouncer(() => clock, 5000);
  const spoken = [];
  for (let files = 0; files <= 203; ++files) { clock += 10; if (announce(files, 203)) spoken.push(files); }
  assert.deepEqual(spoken, [0, 21, 41, 61, 82, 102, 122, 143, 163, 183, 203]);
  const slow = createProgressAnnouncer(() => clock, 5000);
  assert.equal(slow(0, 203), true); clock += 4999; assert.equal(slow(1, 203), false); clock += 1; assert.equal(slow(2, 203), true);
});

test('catalog integrity checks fail clearly without Web Crypto instead of skipping verification', async t => {
  const descriptor = Object.getOwnPropertyDescriptor(globalThis, 'crypto');
  t.after(() => Object.defineProperty(globalThis, 'crypto', descriptor));
  Object.defineProperty(globalThis, 'crypto', { value: {}, configurable: true });
  await assert.rejects(fetchPacked('/', { file: 'x', bytes: 0, sha256: '', contentSha256: '' }, 'fixture'), /HTTPS or localhost/);
  assert.match(integrityUnavailableMessage, /SHA-256/);
});

test('catalog requests resolve only pinned shard file names under the catalog directory', () => {
  const asset = file => ({ file, bytes: 1, sha256: '', contentSha256: '' });
  const snapshot = 'ab'.repeat(32);
  assert.equal(catalogAssetUrl('https://site.test/solar-system-simulator/catalog/', asset('MBA-0009-index.json.gz'), snapshot),
    `https://site.test/solar-system-simulator/catalog/MBA-0009-index.json.gz?snapshot=${snapshot}`);
  assert.match(catalogAssetUrl('/solar-system-simulator/catalog/', asset('overview.json.gz'), snapshot), /\/solar-system-simulator\/catalog\/overview\.json\.gz\?snapshot=/);
  for (const file of ['../manifest.json', 'https://evil.test/x.json.gz', '//evil.test/MBA-0001.json.gz', 'MBA-0001.json.gz?x=1', 'mba-0001.json.gz', 'MBA-0001-index.json', '%2e%2e/MBA-0001.json.gz'])
    assert.throws(() => catalogAssetUrl('https://site.test/catalog/', asset(file), snapshot), /catalog file/i, file);
  assert.throws(() => catalogAssetUrl('https://site.test/catalog/', asset('MBA-0001.json.gz'), 'x&y'), /snapshot/i);
  assert.throws(() => catalogAssetUrl('https://evil.test/catalog/', asset('MBA-0001.json.gz'), snapshot, 'https://site.test/'), /origin/i);
});

test('worker messages are validated strictly before any request is built', () => {
  const manifest = { schema: 2, sourceSha256: 'ab'.repeat(32), epoch: 2461200.5, count: 1, classNames: {}, classes: {},
    overview: { file: 'overview.json.gz', bytes: 1, sha256: 'a', contentSha256: 'b' },
    shards: [{ class: 'MBA', count: 1, minId: 1, maxId: 2, index: { file: 'MBA-0001-index.json.gz', bytes: 1, sha256: 'a', contentSha256: 'b' },
      data: { file: 'MBA-0001.json.gz', bytes: 1, sha256: 'a', contentSha256: 'b' } }] };
  const origin = 'https://site.test';
  const search = { type: 'search', base: `${origin}/catalog/`, manifest, query: 'ceres', group: '', qmax: Infinity, size: '', page: 0, request: 1, allowFullScan: false };
  assert.equal(parseWorkerMessage(search, origin)?.type, 'search');
  assert.equal(parseWorkerMessage({ type: 'record', base: `${origin}/catalog/`, manifest, id: 1, group: 'MBA', request: 2 }, origin)?.type, 'record');
  assert.deepEqual(parseWorkerMessage({ type: 'cancel' }, origin), { type: 'cancel' });
  for (const bad of [null, 'search', { type: 'other' }, { ...search, base: 'https://evil.test/catalog/' }, { ...search, page: -1 }, { ...search, request: 1.5 },
    { ...search, query: 7 }, { ...search, size: 'huge' }, { ...search, qmax: 'x' }, { ...search, query: 'x'.repeat(300) },
    { ...search, manifest: { ...manifest, schema: 1 } }, { ...search, manifest: { ...manifest, sourceSha256: '../x' } },
    { ...search, manifest: { ...manifest, shards: [{ ...manifest.shards[0], index: { ...manifest.shards[0].index, file: '../secret' } }] } },
    { type: 'record', base: `${origin}/catalog/`, manifest, id: 'x', group: 'MBA', request: 2 }])
    assert.equal(parseWorkerMessage(bad, origin), undefined, JSON.stringify(bad)?.slice(0, 120));
});

test('packed assets verify before decoding with or without transparent HTTP decompression', async t => {
  const raw=Buffer.from(JSON.stringify({bodies:1564244})), compressed=gzipSync(raw,{mtime:0});
  const asset={file:'overview.json.gz',bytes:compressed.length,
    sha256:createHash('sha256').update(compressed).digest('hex'),
    contentSha256:createHash('sha256').update(raw).digest('hex')};
  const original=globalThis.fetch;
  t.after(()=>{globalThis.fetch=original;});
  globalThis.fetch=async()=>new Response(compressed);
  assert.deepEqual(await fetchPacked('https://site.test/catalog/',asset,'ab'.repeat(32)),{bodies:1564244});
  globalThis.fetch=async()=>new Response(raw);
  assert.deepEqual(await fetchPacked('https://site.test/catalog/',asset,'ab'.repeat(32)),{bodies:1564244});
  globalThis.fetch=async()=>new Response(Buffer.from('{"bodies":0}'));
  await assert.rejects(fetchPacked('https://site.test/catalog/',asset,'ab'.repeat(32)),/snapshot mismatch/);
});
