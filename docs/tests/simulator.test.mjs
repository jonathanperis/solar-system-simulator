import assert from 'node:assert/strict';
import test from 'node:test';
import { readFile } from 'node:fs/promises';
import { createSimulatorModule, loadRuntimeTextures, filterRuntimeBodies, runtimeBodyFilterStatus, moveSelectByKey, routeSimulatorKeyboard, parsePreparedExperiment, experimentTextLimitBytes } from '../src/lib/simulator.ts';
import { errorMessage, downloadText } from '../src/lib/browser.ts';

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
  const controls = { lesson: { value: '' }, scene: { value: '' }, method: { value: '' }, dt: { value: '' }, factor: { value: '' },
    step: { textContent: '' }, trails: { textContent: '' }, vectors: { textContent: '', setAttribute() {} }, grid: { textContent: '', pressed: '', setAttribute(name, value) { this.pressed = value; } }, labels: { textContent: '', setAttribute() {} }, contact: { textContent: '', hidden: true }, panel: { disabled: false } };
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
  assert.equal(controls.grid.textContent, 'Grid: On');
  runtime.reportLabState({ ...state, grid: false });
  assert.equal(controls.grid.textContent, 'Grid: Off');
  assert.equal(controls.grid.pressed, 'false');
  assert.equal(controls.labels.textContent, 'Labels: On');
  controls.factor.value = '1.2';
  runtime.reportLabState(state);
  assert.equal(controls.factor.value, '1.2');
  runtime.reportLabState({ ...state, factor: 1.3, isolated: false });
  assert.equal(controls.factor.value, '1.3');
  assert.match(readouts.momentum.textContent, /fixed-body constraint/);
  // C reports the loaded lesson's lower speed bound and any contact.
  controls.lesson.value = '6';
  runtime.reportLabState({ ...state, lesson: 6, factor: 0.8, minFactor: 0.73, contactSeconds: 0 });
  assert.equal(controls.factor.min, '0.73');
  assert.doesNotMatch(readouts.scene.textContent, /Contact sphere/);
  runtime.reportLabState({ ...state, lesson: 6, factor: 0.8, minFactor: 0.73, contactSeconds: 5400 });
  assert.match(readouts.scene.textContent, /Contact sphere crossed at 5400 s.*coarse-step artifact/);
});

test('recoverable Emscripten stderr is logged without disabling the runtime; fatal paths still fail', () => {
  const readouts = Object.fromEntries(['status', 'controls', 'elapsed', 'interval', 'parent', 'distance', 'speed', 'mass', 'radius', 'camera', 'achieved', 'pending']
    .map(key => [key, { textContent: '' }]));
  const panels = [{ disabled: true, closest: () => null }];
  const controls = { panels, filterStatus: { textContent: '' }, pause: { textContent: '' }, step: { disabled: true },
    rotate: { checked: true }, body: { value: '', replaceChildren() {} }, speed: { value: '' }, view: { textContent: '' }, search: { value: '' }, group: { value: '' } };
  const runtime = createSimulatorModule({}, readouts, new URL('https://example.test/runtime.js'), controls);
  const logged = [], original = console.warn;
  console.warn = message => logged.push(message);
  try {
    runtime.printErr('wasm streaming compile failed: TypeError: Failed to execute \'compile\' on \'WebAssembly\': Incorrect response MIME type.');
    runtime.printErr('falling back to ArrayBuffer instantiation');
    runtime.printErr('WebGL: INVALID_ENUM: getParameter: invalid parameter name');
  } finally { console.warn = original; }
  assert.equal(logged.length, 3);
  runtime.setStatus('Running...');
  assert.equal(readouts.status.textContent, 'Running...');
  runtime.reportState({ body: 'Sun', parent: 'None', view: 'Illustrative', cameraTarget: 'Sun', selected: 0, paused: false, speedPreset: 1,
    autoRotate: false, elapsedSeconds: 0, intervalSeconds: 300, trailsFailed: false, hasParent: false, distanceM: 0, speedMps: 0,
    massKg: 1.989e30, radiusM: 6.957e8, zoom: 1, massQuality: 0, radiusQuality: 0, achievedTimeScale: 0, pendingSeconds: 0, shortTimescale: false });
  assert.equal(readouts.status.textContent, 'Running physics simulation');
  assert.equal(panels[0].disabled, false);
  runtime.onExit(0);
  assert.equal(panels[0].disabled, false);
  runtime.onExit(1);
  assert.equal(panels[0].disabled, true);
  assert.equal(readouts.status.textContent, 'Runtime error: The simulator stopped (exit status 1). Reload to restart.');
});

test('prepared catalog experiments are untrusted session data bounded by the C parser limit', async () => {
  const header = await readFile(new URL('../../src/sim/experiment.h', import.meta.url), 'utf8');
  assert.equal(experimentTextLimitBytes, Number(header.match(/#define SOLAR_EXPERIMENT_TEXT_BYTES (\d+)/)[1]));
  const text = 'SOLAR_EXPERIMENT_V1 2461200.5\n20000001\tCeres\n';
  assert.deepEqual(parsePreparedExperiment(JSON.stringify({ text, snapshot: 'abc', count: 1 })), { text, snapshot: 'abc', count: 1 });
  for (const [stored, message] of [
    [null, /No prepared experiment/],
    ['{not json', /could not be read/],
    [JSON.stringify({ text, snapshot: 'abc', count: 17 }), /Invalid prepared experiment/],
    [JSON.stringify({ text, snapshot: 7, count: 1 }), /Invalid prepared experiment/],
    [JSON.stringify({ text: 'SOLAR_LAB_V1 circular', snapshot: 'abc', count: 1 }), /Invalid prepared experiment/],
    [JSON.stringify({ text: text + 'x'.repeat(experimentTextLimitBytes), snapshot: 'abc', count: 1 }), /too large/],
    // Multi-byte UTF-8 counts toward the C byte limit, not JavaScript string length.
    [JSON.stringify({ text: text + 'é'.repeat(experimentTextLimitBytes / 2), snapshot: 'abc', count: 1 }), /too large/]
  ]) assert.throws(() => parsePreparedExperiment(stored), message);
});

test('user-facing messages omit raw error prefixes and downloads revoke their URL later', () => {
  assert.equal(errorMessage(new Error('Catalog snapshot mismatch.')), 'Catalog snapshot mismatch.');
  assert.equal(errorMessage(new TypeError('Failed to fetch')), 'Failed to fetch');
  assert.equal(errorMessage('plain text'), 'plain text');
  const revoked = [], clicked = [], scheduled = [];
  const env = {
    createObjectURL: () => 'blob:fixture', revokeObjectURL: url => revoked.push(url),
    createAnchor: () => ({ href: '', download: '', click() { clicked.push([this.href, this.download]); } }),
    schedule: (callback, delay) => scheduled.push([callback, delay])
  };
  downloadText('a,b\n', 'run.csv', 'text/csv', env);
  assert.deepEqual(clicked, [['blob:fixture', 'run.csv']]);
  assert.deepEqual(revoked, []);
  assert.ok(scheduled[0][1] >= 1000);
  scheduled[0][0]();
  assert.deepEqual(revoked, ['blob:fixture']);
});

test('texture loader keeps fallbacks for failed downloads, bad names and exhausted memory', async () => {
  const files = ['sun.jpg', 'mars.jpg', '../evil.jpg', 'moon.jpg'];
  const writes = [], loads = [];
  let nextPointer = 1024, mallocs = 0;
  const runtime = {
    _solar_web_texture_count: () => files.length,
    ccall: (name, result, types, [slot]) => files[slot],
    // moon.jpg makes the second allocation: simulate exhaustion (malloc returns 0).
    _malloc: bytes => (++mallocs === 2 ? 0 : (nextPointer += bytes)),
    _free: () => {},
    HEAPU8: { set: (bytes, pointer) => writes.push(pointer) },
    _solar_web_load_texture: (slot, pointer, length) => { loads.push([slot, length]); return 1; }
  };
  const realFetch = globalThis.fetch, realWarn = console.warn;
  const warnings = [];
  globalThis.fetch = async url => String(url).endsWith('mars.jpg')
    ? new Response('', { status: 404 }) : new Response(new Uint8Array([1, 2, 3]));
  console.warn = message => warnings.push(message);
  try {
    const canvas = { dataset: {} };
    await loadRuntimeTextures(runtime, canvas, new URL('https://example.test/solar/wasm/runtime.js'));
    assert.equal(canvas.dataset.textures, '1/4');
    assert.deepEqual(loads.map(([slot]) => slot), [0]);
    assert.ok(!writes.includes(0), 'never writes texture bytes at address 0');
    assert.ok(warnings.some(w => w.includes('mars.jpg') && w.includes('404')));
    assert.ok(warnings.some(w => w.includes('unexpected texture name')));
    assert.ok(warnings.some(w => w.includes('moon.jpg') && w.includes('out of WebAssembly memory')));
  } finally {
    globalThis.fetch = realFetch;
    console.warn = realWarn;
  }
});
