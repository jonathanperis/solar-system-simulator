import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
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

// Web buttons call the C command boundary with plain integers, so the C
// `SolarCommand` enum and TypeScript `runtimeCommands` must agree exactly.
// Both are parsed from source text: importing simulator.ts would need a DOM.
const commandSource = readFileSync(resolve('src/main.c'), 'utf8');
const enumBody = commandSource.match(/typedef enum SolarCommand\s*\{([^}]*)\}\s*SolarCommand;/);
assert.ok(enumBody, 'src/main.c must define typedef enum SolarCommand { ... } SolarCommand;');
let nextValue = 0;
const cCommands = enumBody[1].replace(/\/\*[\s\S]*?\*\/|\/\/.*$/gm, '').split(',').map(entry => entry.trim()).filter(Boolean)
  .map(entry => {
    const [, name, value] = entry.match(/^SOLAR_COMMAND_([A-Z0-9_]+)(?:\s*=\s*(\d+))?$/) ?? [];
    assert.ok(name, `Unrecognized SolarCommand entry: ${entry}`);
    if (value !== undefined) nextValue = Number(value);
    return [name, nextValue++];
  });
const tsSource = readFileSync(resolve('docs/src/lib/simulator.ts'), 'utf8');
const tsBody = tsSource.match(/export const runtimeCommands\s*=\s*\{([^}]*)\}\s*as const/);
assert.ok(tsBody, 'docs/src/lib/simulator.ts must export runtimeCommands = { ... } as const');
const tsCommands = [...tsBody[1].matchAll(/([A-Za-z_$][\w$]*)\s*:\s*(\d+)/g)]
  .map(([, name, value]) => [name.replace(/([a-z0-9])([A-Z])/g, '$1_$2').toUpperCase(), Number(value)]);
const byValue = ([, a], [, b]) => a - b;
assert.deepEqual(tsCommands.sort(byValue), cCommands.sort(byValue),
  'C SolarCommand values and TypeScript runtimeCommands must match name-for-name');
console.log(`C/TypeScript runtime commands verified: ${cCommands.length} commands`);
