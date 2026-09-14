import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
const file=process.argv[2]??'build/web/catalog-orbits.wasm';
const {instance}=await WebAssembly.instantiate(await readFile(file),{});
instance.exports._initialize?.();
const c=instance.exports.catalog_coordinate, period=instance.exports.catalog_period_days;
const q=1, epoch=2461200.5, AU=149597870700;
assert(Math.abs(c(q,0,0,0,0,epoch,epoch+period(q,0)/4,0)/AU)<1e-9);
assert(Math.abs(c(q,0,0,0,0,epoch,epoch+period(q,0)/4,2)/AU-1)<1e-9);
assert.equal(period(1,1.2),0);
for(const e of [.1,.99,.999999999,1,1.000000001,1.2]) {
  const values=Array.from({length:6},(_,i)=>c(1,e,122,20,80,epoch,epoch+100,i));
  assert(values.every(Number.isFinite));
}
console.log('Standalone C/WASM conic kernel verified');
