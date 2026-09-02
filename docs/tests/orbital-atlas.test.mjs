import assert from 'node:assert/strict';
import test from 'node:test';

import { cycleIndex, nearestBodyIndex, normalizeDegrees } from '../src/lib/orbitalAtlas.ts';

test('V21 normalizes any bearing into one chart revolution', () => {
  assert.equal(normalizeDegrees(-20), 340);
  assert.equal(normalizeDegrees(725), 5);
});

test('V21 selects the nearest body across the zero-degree seam', () => {
  const bodies = [{ chartAngle: 8 }, { chartAngle: 130 }, { chartAngle: 278 }];
  assert.equal(nearestBodyIndex(bodies, 355), 0);
  assert.equal(nearestBodyIndex(bodies, 220), 2);
});

test('V21 cycles body selection in both directions with wraparound', () => {
  assert.equal(cycleIndex(0, -1, 9), 8);
  assert.equal(cycleIndex(8, 1, 9), 0);
});
