import assert from 'node:assert/strict';
import test from 'node:test';
import { plotSegments } from '../src/lib/comparison.ts';

test('plotting preserves unavailable gaps and periodic angle seams', () => {
  assert.equal(plotSegments([{x:0,y:1},{x:1,y:NaN},{x:2,y:2}], [0,2,0,2]).length, 2);
  assert.equal(plotSegments([{x:0,y:179},{x:1,y:-179}], [0,1,-180,180], true).length, 2);
  assert.deepEqual(plotSegments([], [0,1,0,1]), []);
  assert.equal(plotSegments([{x:0,y:0},{x:1,y:1}], [0,1,0,1])[0], '45.00,175.00 480.00,25.00');
});
