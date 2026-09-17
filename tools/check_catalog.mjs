import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { resolve } from 'node:path';
import { implementedBodies } from '../docs/src/lib/bodies.ts';
import { lessonOptions } from '../docs/src/lib/lessonCatalog.ts';

const rows = execFileSync(resolve(process.argv[2] ?? 'build/solar-lab'), ['--catalog'], { encoding: 'utf8' })
  .trim().split('\n').slice(1).map(line => line.split('\t'));
assert.equal(new Set(rows.map(row => row[0])).size, rows.length, 'C IDs must be unique');
assert.deepEqual(rows.map(([, name, kind, parent]) => ({ name, kind, parent })),
  implementedBodies.map(({ name, kind, parent }) => ({ name, kind, parent })),
  'C scene and TypeScript atlas must agree on body order, names, kinds and parents');
console.log(`C/TypeScript core catalog verified: ${rows.length} bodies`);
assert.deepEqual(execFileSync(resolve(process.argv[2] ?? 'build/solar-lab'), ['--lessons'], { encoding: 'utf8' }).trim().split('\n'),
  lessonOptions.map(([name]) => name), 'C and browser lesson ordering must match');
console.log(`C/TypeScript lesson catalog verified: ${lessonOptions.length} presets`);
