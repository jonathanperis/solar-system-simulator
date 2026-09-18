import assert from 'node:assert/strict';
import test from 'node:test';
import { plotSegments, comparisonInputIssue } from '../src/lib/comparison.ts';

test('plotting preserves unavailable gaps and periodic angle seams', () => {
  assert.equal(plotSegments([{x:0,y:1},{x:1,y:NaN},{x:2,y:2}], [0,2,0,2]).length, 2);
  assert.equal(plotSegments([{x:0,y:179},{x:1,y:-179}], [0,1,-180,180], true).length, 2);
  assert.deepEqual(plotSegments([], [0,1,0,1]), []);
  assert.equal(plotSegments([{x:0,y:0},{x:1,y:1}], [0,1,0,1])[0], '45.00,175.00 480.00,25.00');
});

test('rejected comparison settings identify the responsible field without unrelated collision advice', () => {
  const values = { scene: 'circular', factor: '1', methodA: 'verlet', dtA: '300', collisionA: 'none',
    methodB: 'verlet', dtB: '150', collisionB: 'none', sample: '3500', duration: '7200' };
  const issue = comparisonInputIssue(values);
  assert.equal(issue.field, 'sample');
  assert.match(issue.message, /both timesteps.*3600 seconds/);
  assert.doesNotMatch(issue.message, /collision/i);
  assert.equal(comparisonInputIssue({ ...values, sample: '3600', duration: '7100' }).field, 'duration');
  assert.equal(comparisonInputIssue({ ...values, sample: '3600' }), undefined);
  assert.equal(comparisonInputIssue({ ...values, scene: 'collision', dtA: '1' }).field, 'dtA');
  assert.equal(comparisonInputIssue({ ...values, scene: 'core', sample: '3600' }).field, 'dtA');
});
