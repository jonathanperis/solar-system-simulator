import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { familySceneBodies, mainSceneBodies } from '../docs/src/lib/bodies.ts';
import { lessonOptions } from '../docs/src/lib/lessonCatalog.ts';
import { assertCommandsAligned } from './command_ids.mjs';

const lab = resolve(process.argv[2] ?? 'build/solar-lab');
const catalog = (scene) => execFileSync(lab, scene ? ['--catalog', scene] : ['--catalog'], { encoding: 'utf8' })
  .trim().split('\n').slice(1).map(line => line.split('\t'));
const scenes = [[undefined, mainSceneBodies], ['jupiter-system', familySceneBodies('Jupiter')],
  ['saturn-system', familySceneBodies('Saturn')], ['uranus-system', familySceneBodies('Uranus')],
  ['neptune-system', familySceneBodies('Neptune')], ['pluto-system', familySceneBodies('Pluto')],
  ['didymos-system', familySceneBodies('Didymos')]];
for (const [scene, bodies] of scenes) {
  const rows = catalog(scene);
  assert.equal(new Set(rows.map(row => row[0])).size, rows.length, 'C IDs must be unique');
  assert.deepEqual(rows.map(([, name, kind, parent]) => ({ name, kind, parent })),
    bodies.map(({ name, kind, parent }) => ({ name, kind, parent })),
    `C ${scene ?? 'main'} scene and TypeScript catalog must agree on body order, names, kinds and parents`);
  console.log(`C/TypeScript ${scene ?? 'main scene'} catalog verified: ${rows.length} bodies`);
}
assert.deepEqual(execFileSync(resolve(process.argv[2] ?? 'build/solar-lab'), ['--lessons'], { encoding: 'utf8' }).trim().split('\n'),
  lessonOptions.map(([name]) => name), 'C and browser lesson ordering must match');
console.log(`C/TypeScript lesson catalog verified: ${lessonOptions.length} presets`);

const commandCount = assertCommandsAligned(readFileSync(resolve('src/main.c'), 'utf8'),
  readFileSync(resolve('docs/src/lib/simulator.ts'), 'utf8'));
console.log(`C/TypeScript runtime commands verified: ${commandCount} commands`);
