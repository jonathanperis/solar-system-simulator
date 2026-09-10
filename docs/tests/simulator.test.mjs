import assert from 'node:assert/strict';
import test from 'node:test';
import { createSimulatorModule, moveSelectByKey, routeSimulatorKeyboard } from '../src/lib/simulator.ts';

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
  const select = { selectedIndex: 1, options: { length: 3 }, dispatchEvent: event => changes.push(event.type) };
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
});

test('C state drives playback, precise SI readouts, asset pairing, and permanent failure state', () => {
  const readouts = Object.fromEntries(['status', 'controls', 'elapsed', 'interval', 'parent', 'distance', 'speed', 'mass', 'radius', 'camera']
    .map(key => [key, { textContent: '' }]));
  const controls = { panel: { disabled: true }, pause: { textContent: '' }, step: { disabled: true },
    rotate: { checked: true }, body: { value: '' }, speed: { value: '' }, view: { textContent: '' } };
  const runtime = createSimulatorModule({}, readouts,
    new URL('https://example.test/solar-system-simulator/wasm/solar-system-simulator.js?revision=abc123'), controls);
  assert.equal(runtime.locateFile('solar-system-simulator.wasm'),
    'https://example.test/solar-system-simulator/wasm/solar-system-simulator.wasm?revision=abc123');
  const state = { body: 'Phobos', parent: 'Mars', view: 'Illustrative', cameraTarget: 'Mars', selected: 6,
    paused: true, speedPreset: 0, autoRotate: false, elapsedSeconds: 15, intervalSeconds: 300,
    trailsFailed: false, hasParent: true, distanceM: 9233000, speedMps: 2138, massKg: 1.061834e16,
    radiusM: 11266.7, zoom: 0.5 };
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
  assert.equal(controls.panel.disabled, false);
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
  runtime.onAbort('Unable to load WebAssembly');
  runtime.setStatus('');
  runtime.reportState(state);
  assert.equal(readouts.status.textContent, 'Runtime error: Unable to load WebAssembly');
  assert.equal(controls.panel.disabled, true);
});
