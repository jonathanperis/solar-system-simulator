import assert from 'node:assert/strict';
import test from 'node:test';

import { familySceneBodies, implementedBodies, mainSceneBodies, plannedBodies } from '../src/lib/bodies.ts';

test('the catalog lists the main scene, then each system’s small moons', () => {
  assert.equal(mainSceneBodies.length, 32);
  // Main scene, then each system's small moons, plus Didymos and Dimorphos.
  assert.equal(implementedBodies.length, 32 + 111 + 284 + 24 + 15 + 4 + 2);
  assert.deepEqual(implementedBodies[9], {
    slug: 'jupiter', name: 'Jupiter', kind: 'Planet', parent: 'Sun', milestone: 'Outer planet pass',
    initialization: 'Jovian-system barycenter at its Horizons state on 2026-06-09; Jupiter sits opposite its known-mass moons.',
    source: 'src/sim/solar_system.c', summary: 'The largest planet, with its four Galilean moons in the main scene.',
    scene: 'core'
  });
  assert.equal(new Set(implementedBodies.map(body => body.slug)).size, implementedBodies.length);
  assert.deepEqual(plannedBodies, []);
});

test('the main scene keeps the large bodies and family scenes hold every moon', () => {
  assert.equal(mainSceneBodies[14].name, 'Saturn');
  assert.deepEqual(mainSceneBodies.filter(body => body.kind === 'Moon' && !['Earth', 'Mars'].includes(body.parent)).map(body => body.name),
    ['Io', 'Europa', 'Ganymede', 'Callisto', 'Mimas', 'Enceladus', 'Tethys', 'Dione', 'Rhea', 'Titan', 'Iapetus',
      'Ariel', 'Umbriel', 'Titania', 'Oberon', 'Miranda', 'Triton', 'Charon']);
  assert.equal(mainSceneBodies[30].kind, 'Dwarf planet');
  assert.ok(mainSceneBodies.every(body => body.scene === 'core'));
  const counts = { Jupiter: 115, Saturn: 291, Uranus: 29, Neptune: 16, Pluto: 5, Didymos: 1 };
  for (const [planet, count] of Object.entries(counts)) {
    const scene = familySceneBodies(planet);
    // Pluto and Didymos are not planets: they sit at index 9 before their moons.
    const first = ['Pluto', 'Didymos'].includes(planet) ? 10 : 9;
    assert.equal(scene.length, first + count);
    assert.deepEqual(scene.slice(0, 9).map(body => body.name),
      ['Sun', 'Mercury', 'Venus', 'Earth', 'Mars', 'Jupiter', 'Saturn', 'Uranus', 'Neptune']);
    if (first === 10) assert.equal(scene[9].name, planet);
    // Every small moon is reachable: it lives only in its family scene.
    assert.ok(scene.slice(first).every(body => body.parent === planet && (body.scene === 'core' || body.scene === `${planet.toLowerCase()}-system`)));
  }
});

test('every Jovian moon has a unique anchor and explicit data quality', () => {
  const moons = implementedBodies.filter(body => body.parent === 'Jupiter');
  assert.equal(moons.length, 115);
  assert.equal(new Set(moons.map(body => body.slug)).size, 115);
  assert.equal(moons.filter(body => body.group === 'Galilean moons').length, 4);
  assert.equal(moons.filter(body => body.group === 'Inner small moons').length, 4);
  assert.equal(moons.filter(body => body.group === 'Irregular moons').length, 107);
  assert.ok(moons.every(body => body.summary.includes('Mass:')));
  assert.equal(moons.at(-1).name, 'S/2021 J 8');
});
