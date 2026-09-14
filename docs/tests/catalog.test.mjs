import assert from 'node:assert/strict';
import test from 'node:test';
import { createHash } from 'node:crypto';
import { gzipSync } from 'node:zlib';
import { matchesIndex, experimentText, fetchPacked, physicalValues } from '../src/lib/catalog.ts';

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
