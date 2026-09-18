import assert from 'node:assert/strict';
import test from 'node:test';
import { createSimulatorModule, filterRuntimeBodies, runtimeBodyFilterStatus, moveSelectByKey, routeSimulatorKeyboard } from '../src/lib/simulator.ts';

test('browser form keys and Tab bypass GLFW; canvas Space pauses without scrolling', () => {
  for (const [key, focused, reachesGlfw, cancelled] of [
    ['Tab', true, false, false], ['Tab', false, false, false],
    ['Backspace', false, false, false], [' ', false, true, false],
    ['v', false, true, false], ['v', true, true, false], [' ', true, true, true]
  ]) {
    const target = new EventTarget();
    let received = false;
    target.addEventListener('keydown', event => routeSimulatorKeyboard(event, focused));
    target.addEventListener('keydown', () => { received = true; });
    const event = new Event('keydown', { cancelable: true });
    event.key = key;
    target.dispatchEvent(event);
    assert.equal(received, reachesGlfw, `${key}, focused=${focused}`);
    assert.equal(event.defaultPrevented, cancelled);
  }
});

test('select arrow and boundary keys use the same change path as pointer selection', () => {
  const changes = [];
  const select = { selectedIndex: 1, options: Array.from({ length: 3 }, () => ({ disabled: false })), dispatchEvent: event => changes.push(event.type) };
  assert.equal(moveSelectByKey(select, 'ArrowDown'), true);
  assert.equal(select.selectedIndex, 2);
  assert.equal(moveSelectByKey(select, 'ArrowDown'), true);
  assert.equal(select.selectedIndex, 2);
  assert.equal(moveSelectByKey(select, 'Home'), true);
  assert.equal(select.selectedIndex, 0);
  assert.equal(moveSelectByKey(select, 'ArrowUp'), true);
  assert.equal(select.selectedIndex, 0);
  assert.equal(moveSelectByKey(select, 'End'), true);
  assert.equal(select.selectedIndex, 2);
  assert.equal(moveSelectByKey(select, 'x'), false);
  assert.deepEqual(changes, ['change', 'change', 'change']);
  select.options[2].disabled = true;
  assert.equal(moveSelectByKey(select, 'End'), true);
  assert.equal(select.selectedIndex, 1);
});

test('C state drives playback, precise SI readouts, asset pairing, and permanent failure state', () => {
  const readouts = Object.fromEntries(['status', 'controls', 'elapsed', 'interval', 'parent', 'distance', 'speed', 'mass', 'radius', 'camera', 'achieved', 'pending']
    .map(key => [key, { textContent: '' }]));
  let modalClosed = false;
  const controls = { panels: [{ disabled: true, closest: () => null }, { disabled: true, closest: () => ({ close: () => { modalClosed = true; } }) }], filterStatus: { textContent: '' }, pause: { textContent: '' }, step: { disabled: true },
    rotate: { checked: true }, body: { value: '', replaceChildren() {} }, speed: { value: '' }, view: { textContent: '' }, search: { value: '' }, group: { value: '' } };
  const runtime = createSimulatorModule({}, readouts,
    new URL('https://example.test/solar-system-simulator/wasm/solar-system-simulator.js?revision=abc123'), controls);
  assert.equal(runtime.locateFile('solar-system-simulator.wasm'),
    'https://example.test/solar-system-simulator/wasm/solar-system-simulator.wasm?revision=abc123');
  const state = { body: 'Phobos', parent: 'Mars', view: 'Illustrative', cameraTarget: 'Mars', selected: 6,
    paused: true, speedPreset: 0, autoRotate: false, elapsedSeconds: 15, intervalSeconds: 300,
    trailsFailed: false, hasParent: true, distanceM: 9233000, speedMps: 2138, massKg: 1.061834e16,
    radiusM: 11266.7, zoom: 0.5, massQuality: 0, radiusQuality: 0, achievedTimeScale: 864000, pendingSeconds: 43200, shortTimescale: false };
  runtime.reportState(state);
  assert.equal(readouts.status.textContent, 'Simulation paused');
  assert.equal(readouts.elapsed.textContent, '0.00017 simulated days · 15 s');
  assert.equal(readouts.parent.textContent, 'Mars');
  assert.equal(readouts.distance.textContent, '9233.000 km');
  assert.equal(readouts.speed.textContent, '2.138000 km/s');
  assert.equal(readouts.mass.textContent, '1.061834e+16 kg');
  assert.equal(readouts.radius.textContent, '11.267 km');
  assert.equal(controls.body.value, '6');
  assert.equal(controls.speed.value, '0');
  assert.ok(controls.panels.every(panel => !panel.disabled));
  assert.equal(controls.step.disabled, false);
  assert.equal(controls.pause.textContent, 'Resume');
  assert.equal(controls.rotate.checked, false);
  controls.body.value = '7';
  controls.speed.value = '2';
  runtime.reportState(state);
  assert.equal(controls.body.value, '7');
  assert.equal(controls.speed.value, '2');
  runtime.reportState({ ...state, selected: 5, speedPreset: 1 });
  assert.equal(controls.body.value, '5');
  assert.equal(controls.speed.value, '1');
  runtime.reportState({ ...state, paused: false, view: 'Real scale', hasParent: false, parent: 'None' });
  assert.equal(readouts.distance.textContent, 'N/A — no parent');
  assert.equal(readouts.speed.textContent, 'N/A — no parent');
  assert.equal(readouts.radius.textContent, '11.267 km');
  assert.equal(controls.step.disabled, true);
  runtime.reportState({ ...state, selected: 124, speedPreset: 4, paused: false,
    massKg: 0, radiusM: 0, massQuality: 2, radiusQuality: 2 });
  assert.equal(readouts.mass.textContent, 'Unknown (test particle)');
  assert.equal(readouts.radius.textContent, 'Unknown (marker only)');
  assert.equal(readouts.achieved.textContent, '10.00 days / second');
  assert.equal(readouts.pending.textContent, '0.500 days');
  assert.equal(controls.speed.value, '4');
  runtime.reportState({ ...state, massQuality: 1, radiusQuality: 1 });
  assert.equal(readouts.mass.textContent, '1.061834e+16 kg (estimated)');
  assert.equal(readouts.radius.textContent, '11.267 km (estimated)');
  runtime.onAbort('Unable to load WebAssembly');
  runtime.setStatus('');
  runtime.reportState(state);
  assert.equal(readouts.status.textContent, 'Runtime error: Unable to load WebAssembly');
  assert.ok(controls.panels.every(panel => panel.disabled));
  assert.equal(modalClosed, true);
});

test('body filtering matches provisional names and retains the C selection without selecting a different body', () => {
  const bodies = [{ index: 10, name: 'Io', group: 'Galilean moons' },
    { index: 124, name: 'S/2021 J 8', group: 'Irregular moons' },
    { index: 125, name: 'Saturn', group: 'Planets' }];
  assert.deepEqual(filterRuntimeBodies(bodies, '2021 j', '', 10), bodies.slice(0, 2));
  assert.deepEqual(filterRuntimeBodies(bodies, '', 'Galilean moons', 10), [bodies[0]]);
  assert.deepEqual(filterRuntimeBodies(bodies, 'missing', '', 124), [bodies[1]]);
  assert.deepEqual(filterRuntimeBodies(bodies, 'saturn', 'Planets', 125), [bodies[2]]);
  assert.equal(runtimeBodyFilterStatus(bodies, 'missing', '', 124), 'No matching bodies. S/2021 J 8 remains selected.');
  assert.equal(runtimeBodyFilterStatus(bodies, 'saturn', '', 10), '1 matching body. Io remains selected.');
  assert.equal(runtimeBodyFilterStatus(bodies, '', 'Planets', 125), '1 matching body.');
});

test('lesson diagnostics distinguish physical units, normalized energy, and editable pending settings', () => {
  const keys = ['status', 'scene', 'acceleration', 'specificEnergy', 'energy', 'energyChange', 'momentum', 'angularMomentum',
    'integration', 'magnification', 'position', 'velocity'];
  const readouts = Object.fromEntries(keys.map(key => [key, { textContent: '' }]));
  const controls = { lesson: { value: '' }, method: { value: '' }, dt: { value: '' }, factor: { value: '' },
    step: { textContent: '' }, trails: { textContent: '' }, vectors: { textContent: '', setAttribute() {} }, contact: { textContent: '', hidden: true }, panel: { disabled: false } };
  const runtime = createSimulatorModule({}, readouts, new URL('https://example.test/runtime.js'), controls);
  const state = { lesson: 4, method: 1, dt: 30, ticks: 2, factor: 1.1, acceleration: .002,
    specificEnergy: -1000, energy: -2e28, energyChange: .001, isolated: true, momentum: 0, angularMomentum: 4e34,
    magnification: 12, trailFrame: 1, vectors: true, position: [1, 2, 3], velocity: [4, 5, 6], contactMode: 0 };
  runtime.reportLabState(state);
  assert.match(readouts.integration.textContent, /Euler.*30 s.*2/);
  assert.match(readouts.energyChange.textContent, /1.000e-3/);
  assert.match(readouts.momentum.textContent, /isolated/);
  assert.equal(controls.lesson.value, '4');
  assert.equal(controls.step.textContent, 'Step +30 s');
  controls.factor.value = '1.2';
  runtime.reportLabState(state);
  assert.equal(controls.factor.value, '1.2');
  runtime.reportLabState({ ...state, factor: 1.3, isolated: false });
  assert.equal(controls.factor.value, '1.3');
  assert.match(readouts.momentum.textContent, /fixed-body constraint/);
});
