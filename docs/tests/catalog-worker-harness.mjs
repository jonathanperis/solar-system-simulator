import {parentPort} from 'node:worker_threads';
import {readFile} from 'node:fs/promises';
globalThis.self=globalThis;
globalThis.postMessage=message=>parentPort.postMessage(message);
globalThis.fetch=async(url,{signal}={})=>{
  const file=new URL(url).pathname.split('/').at(-1);
  const data=await readFile(new URL(`../public/catalog/${file}`,import.meta.url));
  signal?.throwIfAborted();
  return new Response(data);
};
await import('../src/lib/catalog.worker.ts');
parentPort.on('message',data=>self.onmessage({data}));
parentPort.postMessage({ready:true});
