import { fetchPacked, matchesIndex, type Manifest, type IndexRow } from './catalog.ts';
let generation = 0;
let controller: AbortController | undefined;
// Two bounded shards are sufficient for nearby pages; HTTP caching handles
// repeat scans without retaining a million JS rows in memory.
const cache = new Map<string,IndexRow[]>();
self.onmessage = async event => {
  const token = ++generation;
  controller?.abort();
  controller = new AbortController();
  const signal = controller.signal;
  const {base,manifest,query,group,qmax,size,page,request} = event.data as {
    base:string; manifest:Manifest; query:string; group:string; qmax:number; size:string; page:number; request:number
  };
  const shards = manifest.shards.filter(s => !group || s.class === group);
  const hits: {row:IndexRow; group:string}[] = [];
  const simple = !query.trim() && qmax === Infinity && !size;
  let total = 0;
  try {
    for (let i = 0; i < shards.length; ++i) {
      const shard = shards[i];
      if (simple && total+shard.count <= page*50) {total += shard.count; continue;}
      const key = `${manifest.sourceSha256}/${shard.index.file}`;
      let rows = cache.get(key);
      if (!rows) {
        rows = await fetchPacked<IndexRow[]>(base,shard.index,manifest.sourceSha256,signal);
        if (token !== generation) return;
        cache.set(key,rows);
        if (cache.size > 2) cache.delete(cache.keys().next().value!);
      }
      for (const row of rows) {
        if (!matchesIndex(row,query,qmax,size)) continue;
        if (total >= page*50 && hits.length < 50) hits.push({row,group:shard.class});
        ++total;
      }
      if (simple && hits.length === 50) break;
      self.postMessage({request,progress:`Scanned ${i+1}/${shards.length} index shards`,hits,total,done:false});
    }
    if (token !== generation) return;
    if (simple) total = shards.reduce((n,s) => n+s.count,0);
    self.postMessage({request,hits,total,done:true});
  } catch (error) {
    if (token === generation) self.postMessage({request,error:String(error)});
  }
};
