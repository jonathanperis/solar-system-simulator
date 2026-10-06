import assert from 'node:assert/strict';
import { readdir, readFile } from 'node:fs/promises';
import { execFileSync } from 'node:child_process';
import { resolve } from 'node:path';
import { pathToFileURL } from 'node:url';

const path = resolve(process.argv[2] ?? 'build/web/learning-lab.mjs');
const { default: createLab } = await import(pathToFileURL(path));
const lab = await createLab();
// Before any configuration nothing can advance: 0 tells a JS loop to stop
// instead of spinning on "still running".
assert.equal(lab._lab_advance(8192, 0), 0, 'unconfigured lab_advance must report no work');
const minimumFactor = name => lab.ccall('lab_minimum_velocity_factor', 'number', ['string'], [name]);
assert(Number.isFinite(minimumFactor('circular')) && minimumFactor('circular') > 0, 'known lesson has a minimum factor');
assert(Number.isNaN(minimumFactor('no-such-lesson')), 'unknown lesson has no minimum factor');
// Replay every shipped example so a new lesson descriptor cannot skip parity.
const examples = (await readdir(resolve('examples')))
  .filter(name => name.endsWith('.solar'))
  .map(name => name.slice(0, -'.solar'.length))
  .sort();
assert(examples.length > 0, 'examples/*.solar must not be empty');
for (const example of examples) {
  const filename = resolve(`examples/${example}.solar`);
  const definition = await readFile(filename, 'utf8');
  assert.equal(lab.ccall('lab_start', 'number', ['string'], [definition]), 1);
  let batches = 0;
  while (!lab._lab_status(1) && !lab._lab_status(2)) {
    lab._lab_advance(8192, 0);
    assert(++batches < 100000, 'bounded fixture must complete');
  }
  assert.equal(lab._lab_status(2), 0);
  const exported = lab.ccall('lab_export_csv', 'string', [], []);
  const native = execFileSync(resolve('build/solar-lab'), ['--compare', filename], { encoding: 'utf8', maxBuffer: 256e6 });
  const rows = text => text.trim().split('\n').filter(line => !line.startsWith('#')).slice(1);
  const actual = rows(exported).at(-1).split(',');
  const expected = rows(native).at(-1).split(',');
  assert.equal(actual.length, expected.length);
  actual.forEach((value, i) => {
    if (expected[i] === '') assert.equal(value, '', `${example} unavailable field ${i}`);
    else assert(Math.abs(Number(value) - Number(expected[i])) <= 1e-7 + Math.abs(Number(expected[i])) * 1e-10,
      `${example} field ${i}: native ${expected[i]}, WASM ${value}`);
  });
}
assert.equal(lab.ccall('lab_start', 'number', ['string'], ['SOLAR_LAB_V99 invalid']), 0);
console.log(`Comparison C/native/WASM replay verified: ${examples.join(', ')}.`);
