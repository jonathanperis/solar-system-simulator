import assert from 'node:assert/strict';
import test from 'node:test';
import { createHash } from 'node:crypto';
import { gzipSync } from 'node:zlib';
import { matchesIndex, experimentText, fetchPacked, physicalValues, numericIdentityCandidates, routeShards, ColumnarIndex, columnarRowLimit, integrityUnavailableMessage } from '../src/lib/catalog.ts';

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

test('catalog integrity checks fail clearly without Web Crypto instead of skipping verification', async t => {
  const descriptor = Object.getOwnPropertyDescriptor(globalThis, 'crypto');
  t.after(() => Object.defineProperty(globalThis, 'crypto', descriptor));
  Object.defineProperty(globalThis, 'crypto', { value: {}, configurable: true });
  await assert.rejects(fetchPacked('/', { file: 'x', bytes: 0, sha256: '', contentSha256: '' }, 'fixture'), /HTTPS or localhost/);
  assert.match(integrityUnavailableMessage, /SHA-256/);
});

test('packed assets verify before decoding with or without transparent HTTP decompression', async t => {
  const raw=Buffer.from(JSON.stringify({bodies:1564244})), compressed=gzipSync(raw,{mtime:0});
  const asset={file:'fixture.json.gz',bytes:compressed.length,
    sha256:createHash('sha256').update(compressed).digest('hex'),
    contentSha256:createHash('sha256').update(raw).digest('hex')};
  const original=globalThis.fetch;
  t.after(()=>{globalThis.fetch=original;});
  globalThis.fetch=async()=>new Response(compressed);
  assert.deepEqual(await fetchPacked('/',asset,'fixture'),{bodies:1564244});
  globalThis.fetch=async()=>new Response(raw);
  assert.deepEqual(await fetchPacked('/',asset,'fixture'),{bodies:1564244});
  globalThis.fetch=async()=>new Response(Buffer.from('{"bodies":0}'));
  await assert.rejects(fetchPacked('/',asset,'fixture'),/snapshot mismatch/);
});
