import assert from 'node:assert/strict';
import test from 'node:test';
import {Worker} from 'node:worker_threads';
import manifest from '../public/catalog/manifest.json' with {type:'json'};

test('real full-catalog search stays paginated and a superseded request cannot publish final results',async()=>{
  const worker=new Worker(new URL('./catalog-worker-harness.mjs',import.meta.url));
  const results=[];
  try {
    await new Promise((resolve,reject)=>{worker.once('error',reject);worker.once('message',resolve);});
    worker.on('message',m=>{results.push(m);if(m.hits)assert(m.hits.length<=50);});
    const query=(request,overrides={})=>({base:'https://catalog.test/',manifest,query:'',group:'',qmax:Infinity,size:'',page:0,request,...overrides});
    const done=request=>new Promise((resolve,reject)=>{
      const listener=m=>{if(m.request===request&&(m.done||m.error)){worker.off('message',listener);m.error?reject(new Error(m.error)):resolve(m);}};
      worker.on('message',listener);
    });
    let pending=done(1);worker.postMessage(query(1));let result=await pending;
    assert.equal(result.total,manifest.count);assert.equal(result.hits.length,50);
    pending=done(2);worker.postMessage(query(2,{query:'Vesta'}));result=await pending;
    assert(result.hits.some(hit=>hit.row[0]===20000004));
    pending=done(4);worker.postMessage(query(3,{query:'Pallas'}));worker.postMessage(query(4,{query:'Pluto',group:'TNO',size:'known'}));result=await pending;
    assert(result.hits.some(hit=>hit.row[0]===20134340));
    assert(!results.some(m=>m.request===3&&m.done));
  } finally {await worker.terminate();}
});
