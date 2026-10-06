// Proves the command-ID cross-check fails on drift, not just that it passes today.
import assert from 'node:assert/strict';
import { assertCommandsAligned } from './command_ids.mjs';

const c = 'typedef enum SolarCommand { SOLAR_COMMAND_PAUSE, SOLAR_COMMAND_STEP, /* note */ SOLAR_COMMAND_FRAME_BODY = 5 } SolarCommand;';
const ts = 'export const runtimeCommands = { pause: 0, step: 1, frameBody: 5 } as const';
assert.equal(assertCommandsAligned(c, ts), 3);
for (const [label, drifted] of [
  ['swapped values', ts.replace('pause: 0, step: 1', 'pause: 1, step: 0')],
  ['extra TypeScript entry', ts.replace('frameBody: 5', 'frameBody: 5, reset: 6')],
  ['missing TypeScript entry', ts.replace(', frameBody: 5', '')],
  ['renamed entry', ts.replace('frameBody', 'frameSystem')]
]) assert.throws(() => assertCommandsAligned(c, drifted), /must match/, label);
assert.throws(() => assertCommandsAligned('enum Other { A };', ts), /must define/);
console.log('Command-ID cross-check rejects drift: 4 mismatch cases and a missing enum.');
