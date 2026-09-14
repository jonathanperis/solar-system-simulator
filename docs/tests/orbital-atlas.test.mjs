import assert from 'node:assert/strict';
import test from 'node:test';

import { cycleIndex, nearestBodyIndex, normalizeDegrees } from '../src/lib/orbitalAtlas.ts';
import { implementedBodies, plannedBodies } from '../src/lib/bodies.ts';

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
  assert.equal(cycleIndex(0, -1, 10), 9);
  assert.equal(cycleIndex(9, 1, 10), 0);
});

test('A10 publishes Jupiter as the tenth implemented atlas body', () => {
  assert.equal(implementedBodies.length, 128);
  assert.deepEqual(implementedBodies[9], {
    slug: 'jupiter',
    name: 'Jupiter',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Outer planet pass',
    initialization: 'Planar heliocentric perihelion position with vis-viva tangential speed.',
    source: 'src/sim/solar_system.c',
    accent: 'jupiter',
    chart: { plate: 'heliocentric', angle: 112, radius: 91 },
    summary: 'First gas giant, initialized at heliocentric perihelion.'
  });
});

test('Saturn is the appended heliocentric milestone', () => {
  assert.deepEqual(implementedBodies[125], {
    slug: 'saturn',
    name: 'Saturn',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Saturn pass',
    initialization: 'Planar heliocentric perihelion position with vis-viva tangential speed.',
    source: 'src/sim/solar_system.c',
    accent: 'saturn',
    chart: { plate: 'heliocentric', angle: 196, radius: 97 },
    summary: 'Ringed gas giant initialized at heliocentric perihelion; rings are renderer-only.'
  });
  assert.equal(plannedBodies[0], 'complete Saturnian moons');
});

test('the Jovian atlas exposes every sourced moon with unique anchors and explicit data quality', () => {
  const moons = implementedBodies.filter(body => body.parent === 'Jupiter');
  assert.equal(moons.length, 115);
  assert.equal(new Set(moons.map(body => body.slug)).size, 115);
  assert.equal(moons.filter(body => body.group === 'Galilean moons').length, 4);
  assert.equal(moons.filter(body => body.group === 'Inner small moons').length, 4);
  assert.equal(moons.filter(body => body.group === 'Irregular moons').length, 107);
  assert.ok(moons.every(body => body.chart.plate === 'jupiter' && body.summary.includes('Mass:')));
  assert.equal(moons.at(-1).name, 'S/2021 J 8');
});
