import assert from 'node:assert/strict';
import test from 'node:test';
import { plotSegments, comparisonInputIssue, sharedRevisionLabel } from '../src/lib/comparison.ts';

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
  // C's lesson minimum explains a too-slow start at the factor field.
  assert.equal(comparisonInputIssue({ ...values, sample: '3600', scene: 'phobos', factor: '0.5' }, 0.73).field, 'factor');
  assert.match(comparisonInputIssue({ ...values, sample: '3600', scene: 'phobos', factor: '0.5' }, 0.73).message, /0\.73.*parent body/);
  assert.equal(comparisonInputIssue({ ...values, sample: '3600', scene: 'phobos', factor: '0.73' }, 0.73), undefined);
  assert.equal(comparisonInputIssue({ ...values, scene: 'collision', dtA: '1' }).field, 'dtA');
  assert.equal(comparisonInputIssue({ ...values, scene: 'core', sample: '3600' }).field, 'dtA');
});

test('shared-link revisions are shown only when they look like a commit hash', () => {
  assert.equal(sharedRevisionLabel(null), 'not recorded');
  assert.equal(sharedRevisionLabel('4cdffdc'), '4cdffdc');
  assert.equal(sharedRevisionLabel('ad43708e9f1c2b3a4d5e6f708192a3b4c5d6e7f8'), 'ad43708e9f1c2b3a4d5e6f708192a3b4c5d6e7f8');
  assert.equal(sharedRevisionLabel('ad43708-dirty'), 'ad43708-dirty');
  for (const value of ['', 'abc', 'Click here to claim', 'ad43708; rm -rf', 'AD43708', 'g'.repeat(7), 'a'.repeat(41)])
    assert.equal(sharedRevisionLabel(value), 'unrecognized revision', value);
});
