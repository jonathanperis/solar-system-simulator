import assert from 'node:assert/strict';
import test from 'node:test';
import {Worker} from 'node:worker_threads';
import manifest from '../public/catalog/manifest.json' with {type:'json'};
import {routeShards} from '../src/lib/catalog.ts';

function startWorker() {
  const worker=new Worker(new URL('./catalog-worker-harness.mjs',import.meta.url));
  const messages=[];
  worker.on('message',m=>{messages.push(m);if(m.hits)assert(m.hits.length<=50);});
  const ready=new Promise((resolve,reject)=>{worker.once('error',reject);worker.once('message',resolve);});
  const search=(request,overrides={})=>({type:'search',base:'https://catalog.test/solar-system-simulator/catalog/',manifest,query:'',group:'',qmax:Infinity,size:'',page:0,request,...overrides});
  const reply=(predicate)=>new Promise((resolve)=>{
    const listener=m=>{if(predicate(m)){worker.off('message',listener);resolve(m);}};
    worker.on('message',listener);
  });
  const done=request=>reply(m=>m.type==='search'&&m.request===request&&m.done).then(m=>{if(m.error)throw new Error(m.error);return m;});
  const fetched=async()=>{const pending=reply(m=>m.harness==='fetches');worker.postMessage({harness:'fetches'});return (await pending).files;};
  return {worker,messages,ready,search,reply,done,fetched};
}

test('identity queries route to owning shards; text needs consent; the full scan is built once and reused', async()=>{
  const {worker,messages,ready,search,done,fetched}=startWorker();
  try {
    await ready;
    let pending=done(1);worker.postMessage(search(1));let result=await pending;
    assert.equal(result.total,manifest.count);assert.equal(result.hits.length,50);
    assert.deepEqual(await fetched(),[manifest.shards[0].index.file]);

    pending=done(2);worker.postMessage(search(2,{query:'433'}));result=await pending;
    assert.deepEqual(result.hits.map(hit=>[hit.id,hit.name,hit.group]),[[20000433,'433 Eros (A898 PA)','AMO']]);
    const routed=routeShards(manifest,[20000433],'').map(s=>s.index.file);
    // The first AMO shard is already in the LRU from the browsing request above.
    assert.deepEqual(await fetched(),routed.filter(file=>file!==manifest.shards[0].index.file));
    assert(routed.length<=13);

    pending=done(3);worker.postMessage(search(3,{query:'Vesta'}));result=await pending;
    assert.deepEqual(result.needsFullScan,{bytes:manifest.shards.reduce((n,s)=>n+s.index.bytes,0),files:manifest.shards.length});
    assert.deepEqual(await fetched(),[],'no download before the user accepts the full scan');

    pending=done(5);
    worker.postMessage(search(4,{query:'Vesta',allowFullScan:true}));
    // A filter change during the accepted download supersedes the request but keeps the download.
    worker.postMessage(search(5,{query:'Pallas'}));
    result=await pending;
    assert(result.hits.some(hit=>hit.id===20000002&&hit.name==='2 Pallas (A802 FA)'));
    assert(!messages.some(m=>m.request===4&&m.done),'superseded request published final results');
    assert(messages.some(m=>m.request===5&&/Downloading the catalog index: \d+ of 203 files/.test(m.progress??'')));
    assert.equal((await fetched()).length,manifest.shards.length);

    for (const [request,overrides,expect] of [
      [6,{query:'Pluto',group:'TNO',size:'known'},hits=>hits.some(hit=>hit.id===20134340&&hit.group==='TNO')],
      [7,{query:'vesta'},hits=>hits.some(hit=>hit.id===20000004&&hit.name==='4 Vesta (A807 FA)')],
      [8,{query:'433',qmax:1.2},hits=>hits.length===1&&hits[0].perihelion<1.2],
      [9,{query:'',qmax:0.1},hits=>hits.every(hit=>hit.perihelion<=0.1)]
    ]) {
      pending=done(request);worker.postMessage(search(request,overrides));result=await pending;
      assert.equal(result.source,'memory');assert(expect(result.hits),JSON.stringify(overrides));
    }
    pending=done(10);worker.postMessage(search(10,{query:'',size:'unknown',page:3}));result=await pending;
    assert.equal(result.hits.length,50);assert(result.total>100000);
    assert.deepEqual(await fetched(),[],'repeated searches and filter changes refetch nothing');
  } finally {await worker.terminate();}
});

test('an explicit cancel stops the full download and record lookups reuse a small data-shard cache', async()=>{
  const {worker,ready,search,reply,done,fetched}=startWorker();
  try {
    await ready;
    let pending=done(1);
    worker.postMessage(search(1,{query:'Ceres',allowFullScan:true}));
    await reply(m=>m.request===1&&m.progress);
    worker.postMessage({type:'cancel'});
    const result=await pending;
    assert.equal(result.cancelled,true);assert.equal(result.hits,undefined);
    assert((await fetched()).length<manifest.shards.length);

    const record=(request,id,group)=>{const answer=reply(m=>m.type==='record'&&m.request===request);
      worker.postMessage({type:'record',base:'https://catalog.test/solar-system-simulator/catalog/',manifest,id,group,request});return answer;};
    const ceres=await record(2,20000001,'MBA');
    assert.equal(ceres.record[1],'1 Ceres (A801 AA)');
    const first=await fetched();
    assert.equal(first.length,1);assert.doesNotMatch(first[0],/index/);
    const pallas=await record(3,20000002,'MBA');
    assert.equal(pallas.record[1],'2 Pallas (A802 FA)');
    assert.deepEqual(await fetched(),[],'same data shard served from the LRU');
    const missing=await record(4,99999999,'MBA');
    assert.match(missing.error,/not present/);

    pending=done(5);worker.postMessage(search(5,{query:'Ceres',allowFullScan:true}));
    assert((await pending).hits.some(hit=>hit.id===20000001),'a later confirmed search restarts the download');
  } finally {await worker.terminate();}
});

test('the worker rejects messages that would steer requests off the catalog directory', async()=>{
  const {worker,ready,search,reply,fetched}=startWorker();
  try {
    await ready;
    const hostileShard={...manifest.shards[0],index:{...manifest.shards[0].index,file:'../../secrets.json'}};
    for (const [request,overrides] of [
      [1,{base:'https://evil.test/catalog/'}],
      [2,{base:'https://catalog.test/solar-system-simulator/'}],
      [3,{manifest:{...manifest,shards:[hostileShard]}}],
      [4,{page:-1}]
    ]) {
      const answer=reply(m=>m.request===request&&m.done);
      worker.postMessage(search(request,overrides));
      assert.match((await answer).error,/Invalid catalog request/);
    }
    assert.deepEqual(await fetched(),[]);
  } finally {await worker.terminate();}
});
