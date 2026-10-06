import {parentPort} from 'node:worker_threads';
import {readFile} from 'node:fs/promises';
globalThis.self=globalThis;
// A dedicated worker's location; catalog requests must stay on this origin.
globalThis.location=new URL('https://catalog.test/solar-system-simulator/_astro/catalog.worker.js');
globalThis.postMessage=message=>parentPort.postMessage(message);
// Count every catalog file request so tests can prove routing and cache reuse.
let fetches=[];
globalThis.fetch=async(url,{signal}={})=>{
  const file=new URL(url).pathname.split('/').at(-1);
  fetches.push(file);
  const data=await readFile(new URL(`../public/catalog/${file}`,import.meta.url));
  signal?.throwIfAborted();
  return new Response(data);
};
await import('../src/lib/catalog.worker.ts');
parentPort.on('message',data=>{
  if(data?.harness==='fetches'){parentPort.postMessage({harness:'fetches',files:fetches});fetches=[];return;}
  self.onmessage({data,origin:''});
});
parentPort.postMessage({ready:true});
