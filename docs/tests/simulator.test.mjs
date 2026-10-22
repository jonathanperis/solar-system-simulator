import assert from 'node:assert/strict';
import test from 'node:test';
import { readFile } from 'node:fs/promises';
import { createSimulatorModule, loadRuntimeTextures, filterRuntimeBodies, runtimeBodyFilterStatus, moveSelectByKey, routeSimulatorKeyboard, parsePreparedExperiment, experimentTextLimitBytes, sceneDate, sceneSecondsAt, sceneSecondsAtDate, dateRunNote, dateRunArrived, sceneSecondsAtLink, linkForSceneSeconds, clockNote, speedLabels, findMatches, formatSpacing, formatDistance, formatSpeed, formatMass, formatRadius, formatPeriod, formatFromEarth } from '../src/lib/simulator.ts';
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
    rotate: { attrs: {}, setAttribute(name, value) { this.attrs[name] = value; }, getAttribute(name) { return this.attrs[name] ?? null; } }, body: { value: '', replaceChildren() {} }, speed: { value: '' }, search: { value: '' }, group: { value: '' } };
  const runtime = createSimulatorModule({}, readouts,
    new URL('https://example.test/solar-system-simulator/wasm/solar-system-simulator.js?revision=abc123'), controls);
  assert.equal(runtime.locateFile('solar-system-simulator.wasm'),
    'https://example.test/solar-system-simulator/wasm/solar-system-simulator.wasm?revision=abc123');
  const state = { body: 'Phobos', parent: 'Mars', view: 'Illustrative', cameraTarget: 'Mars', selected: 6,
    paused: true, speedPreset: 0, autoRotate: false, elapsedSeconds: 15, intervalSeconds: 300,
    trailsFailed: false, hasParent: true, distanceM: 9233000, speedMps: 2138, massKg: 1.061834e16,
    radiusM: 11266.7, zoom: 0.5, massQuality: 0, radiusQuality: 0, achievedTimeScale: 864000, pendingSeconds: 43200, shortTimescale: false };
  runtime.reportState(state);
  assert.equal(readouts.status.textContent, 'Paused');
  assert.equal(readouts.elapsed.textContent, '0.0002 d (15 s)');
  assert.equal(readouts.parent.textContent, 'Mars');
  assert.equal(readouts.distance.textContent, '9\u202f233.0\u00a0km');
  assert.equal(readouts.speed.textContent, '2.1380\u00a0km/s');
  assert.equal(readouts.mass.textContent, '1.0618\u00a0×\u00a010¹⁶\u00a0kg');
  assert.equal(readouts.radius.textContent, '11.267\u00a0km');
  assert.equal(controls.body.value, '6');
  assert.equal(controls.speed.value, '0');
  assert.ok(controls.panels.every(panel => !panel.disabled));
  assert.equal(controls.step.disabled, false);
  assert.equal(controls.pause.textContent, 'Resume');
  assert.equal(controls.rotate.getAttribute('aria-checked'), 'false');
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
  assert.equal(readouts.radius.textContent, '11.267\u00a0km');
  assert.equal(controls.step.disabled, true);
  runtime.reportState({ ...state, selected: 124, speedPreset: 4, paused: false,
    massKg: 0, radiusM: 0, massQuality: 2, radiusQuality: 2 });
  assert.equal(readouts.mass.textContent, 'Unknown (test particle)');
  assert.equal(readouts.radius.textContent, 'Unknown (marker only)');
  assert.equal(readouts.achieved.textContent, '10.00 days / second');
  assert.equal(readouts.pending.textContent, '0.500 days');
  assert.equal(controls.speed.value, '4');
  runtime.reportState({ ...state, massQuality: 1, radiusQuality: 1 });
  assert.equal(readouts.mass.textContent, '1.0618\u00a0×\u00a010¹⁶\u00a0kg (estimated)');
  assert.equal(readouts.radius.textContent, '11.267\u00a0km (estimated)');
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
    step: { textContent: '' }, vectors: { attrs: {}, setAttribute(name, value) { this.attrs[name] = value; }, getAttribute(name) { return this.attrs[name] ?? null; } }, grid: { attrs: {}, setAttribute(name, value) { this.attrs[name] = value; }, getAttribute(name) { return this.attrs[name] ?? null; } }, labels: { attrs: {}, setAttribute(name, value) { this.attrs[name] = value; }, getAttribute(name) { return this.attrs[name] ?? null; } }, orbits: { attrs: {}, setAttribute(name, value) { this.attrs[name] = value; }, getAttribute(name) { return this.attrs[name] ?? null; } }, contact: { textContent: '', hidden: true }, panel: { disabled: false } };
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
  // A128: on/off controls are switches; their names never change.
  assert.equal(controls.grid.getAttribute('aria-checked'), 'true');
  runtime.reportLabState({ ...state, grid: false });
  assert.equal(controls.grid.getAttribute('aria-checked'), 'false');
  assert.equal(controls.labels.getAttribute('aria-checked'), 'true');
  assert.equal(controls.orbits.getAttribute('aria-checked'), 'true');
  runtime.reportLabState({ ...state, orbits: false });
  assert.equal(controls.orbits.getAttribute('aria-checked'), 'false');
  assert.equal(controls.vectors.getAttribute('aria-checked'), 'true');
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
    rotate: { attrs: {}, setAttribute(name, value) { this.attrs[name] = value; }, getAttribute(name) { return this.attrs[name] ?? null; } }, body: { value: '', replaceChildren() {} }, speed: { value: '' }, search: { value: '' }, group: { value: '' } };
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
  assert.equal(readouts.status.textContent, 'Running');
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

test('the belt loads once whether texture setup succeeds or gives up early (SPEC A131)', async () => {
  const canvas = { dataset: {} };
  let calls = 0;
  const belt = async () => { calls++; };
  // No texture helpers at all: setup gives up, the belt still starts.
  await loadRuntimeTextures({}, canvas, new URL('https://example.test/wasm/runtime.js'), belt);
  assert.equal(calls, 1);
  // A normal run starts it exactly once, between the waves; a failing
  // callback never breaks the loader.
  const realFetch = globalThis.fetch;
  globalThis.fetch = async () => new Response(new Uint8Array([1, 2, 3]));
  try {
    const runtime = {
      _solar_web_texture_count: () => 2, ccall: (name, result, types, [slot]) => ['sun.jpg', 'mercury.jpg'][slot],
      _malloc: bytes => bytes, _free: () => {}, HEAPU8: { set: () => {} }, _solar_web_load_texture: () => 1,
    };
    calls = 0;
    await loadRuntimeTextures(runtime, canvas, new URL('https://example.test/wasm/runtime.js'), belt);
    assert.equal(calls, 1);
    await loadRuntimeTextures(runtime, canvas, new URL('https://example.test/wasm/runtime.js'), async () => { throw new Error('belt failed'); });
    assert.equal(canvas.dataset.textures, '2/2');
  } finally {
    globalThis.fetch = realFetch;
  }
});

test('the sky clock reads the scene date and how far the run has come', () => {
  assert.equal(sceneDate(0), '2026-06-09 00:00 TDB');
  assert.equal(sceneDate(38.84 * 86400), '2026-07-17 20:09 TDB');
  assert.equal(clockNote(38.84 * 86400, true), '+38.84 days since 2026-06-09');
  assert.equal(clockNote(0, true), '+0.00 days since 2026-06-09');
  // Lessons are not dated: their clock counts from the lesson start.
  assert.equal(clockNote(140970, false), 'since the lesson started');
});

test('Today runs to the present in TDB, as the C helper does (SPEC A133)', () => {
  const epochMs = Date.UTC(2026, 5, 9);
  assert.ok(Math.abs(sceneSecondsAt(epochMs) - 69.184) < 1e-6);
  // At 2026-10-21 12:00 UTC the clock reads 12:01 TDB.
  const seconds = sceneSecondsAt(Date.UTC(2026, 9, 21, 12));
  assert.equal(sceneDate(seconds), '2026-10-21 12:01 TDB');
});

test('inspector readouts group digits and carry readable units (SPEC A113)', () => {
  const nb = '\u202f';
  assert.equal(formatDistance(1.41648818261e12), `1${nb}416${nb}488${nb}183\u00a0km · 9.4686\u00a0AU`);
  assert.equal(formatDistance(384399.5e3), `384${nb}399.5\u00a0km`);
  assert.equal(formatDistance(9233e3), `9${nb}233.0\u00a0km`);
  assert.equal(formatDistance(1189.4), '1.189\u00a0km');
  assert.equal(formatSpeed(9716.28), '9.7163\u00a0km/s');
  assert.equal(formatSpeed(213.8), '213.80\u00a0m/s');
  assert.equal(formatSpeed(0.17), '0.170\u00a0m/s');
  assert.equal(formatMass(5.683174e26), '5.6832\u00a0×\u00a010²⁶\u00a0kg');
  assert.equal(formatMass(1.061834e16), '1.0618\u00a0×\u00a010¹⁶\u00a0kg');
  assert.equal(formatMass(4.2e9), '4.2000\u00a0×\u00a010⁹\u00a0kg');
  assert.equal(formatRadius(58232e3), `58${nb}232.0\u00a0km`);
  assert.equal(formatRadius(11267), '11.267\u00a0km');
  assert.equal(formatPeriod(10765.1427 * 86400), `10${nb}765.14\u00a0d · 29.47\u00a0yr`);
  assert.equal(formatPeriod(27.3217 * 86400), '27.322\u00a0d');
  assert.equal(formatPeriod(13.942 * 3600), '13.942\u00a0h');
});

test('arrow keys reach the simulator without scrolling, and the speed slider reads its preset (SPEC A120)', () => {
  for (const [key, focused, prevented] of [['ArrowLeft', true, true], ['ArrowDown', true, true], ['ArrowLeft', false, false], ['a', true, false]]) {
    let cancelled = false, stopped = false;
    routeSimulatorKeyboard({ key, preventDefault() { cancelled = true; }, stopImmediatePropagation() { stopped = true; } }, focused);
    assert.equal(cancelled, prevented, `${key}, focused=${focused}`);
    assert.equal(stopped, false);
  }
  assert.deepEqual(speedLabels(false).short, ['1 h/s', '1 d/s', '5 d/s', '10 d/s', '15 d/s']);
  assert.equal(speedLabels(true).spoken[4], '50 seconds / second');
});

test('Find lists names that start with the query first, then ones containing it (SPEC A130)', () => {
  const bodies = [
    { index: 0, name: 'Sun', group: 'Star' }, { index: 21, name: 'Titan', group: 'Major moons' },
    { index: 24, name: 'Titania', group: 'Major moons' }, { index: 40, name: 'Metis', group: 'Inner small moons' },
    { index: 41, name: 'Mimas', group: 'Major moons' },
  ];
  assert.deepEqual(findMatches(bodies, 'ti', '').map(body => body.name), ['Titan', 'Titania', 'Metis']);
  assert.deepEqual(findMatches(bodies, 'ti', 'Inner small moons').map(body => body.name), ['Metis']);
  assert.deepEqual(findMatches(bodies, '   ', ''), []);
  assert.equal(findMatches(Array.from({ length: 20 }, (_, i) => ({ index: i, name: `Moon ${i}`, group: 'g' })), 'moon', '').length, 8);
  assert.equal(formatSpacing(300), '300\u00a0s between trail samples');
  assert.equal(formatSpacing(0.1), '0.1\u00a0s between trail samples');
  assert.equal(formatSpacing(12.345), '12.3\u00a0s between trail samples');
  assert.equal(formatSpacing(7200), '2.00\u00a0h between trail samples');
});

test('Go to date reads a TDB instant and says how the run will go (SPEC A134)', () => {
  assert.equal(sceneSecondsAtDate('2026-06-09', '00:00'), 0);
  assert.equal(sceneSecondsAtDate('2026-06-10', '06:30'), 86400 + 6.5 * 3600);
  assert.equal(sceneDate(sceneSecondsAtDate('2031-02-28', '23:59')), '2031-02-28 23:59 TDB');
  // Before the epoch, past the runnable range, or malformed: not a target.
  assert.ok(Number.isNaN(sceneSecondsAtDate('2026-06-08', '23:59')));
  assert.ok(Number.isNaN(sceneSecondsAtDate('2077-01-01', '00:00')));
  // The whole last day is runnable; the next one is not (Bugbot).
  assert.ok(Number.isFinite(sceneSecondsAtDate('2076-06-08', '23:59')));
  assert.ok(Number.isNaN(sceneSecondsAtDate('2076-06-09', '00:00')));
  assert.ok(sceneSecondsAtDate('2076-06-08', '23:59') < 2 ** 31);
  assert.ok(Number.isNaN(sceneSecondsAtDate('', '00:00')));
  assert.equal(dateRunNote(135 * 86400, 0), 'Plays forward about 9 s at 15 d/s.');
  assert.equal(dateRunNote(10 * 365.25 * 86400, 0), 'Plays forward about 4 min at 15 d/s.');
  assert.equal(dateRunNote(86400, 2 * 86400), 'Restarts at 2026-06-09, then plays under a second.');
  assert.equal(dateRunNote(NaN, 0), 'Choose a date from 2026-06-09 to 2076-06-08.');
  // Date opened right after Today shows the clock's own minute, a few
  // seconds behind it: that is arrival, not a restart (Bugbot).
  assert.ok(dateRunArrived(86400, 86400 + 45));
  assert.ok(!dateRunArrived(86400, 86400 + 60));
  assert.ok(!dateRunArrived(86400 + 15, 86400));
  assert.equal(dateRunNote(86400, 86400 + 45), 'The clock is already there.');
});

test('From Earth gives the distance and the light time (SPEC A136)', () => {
  const nb = '\u00a0', nn = '\u202f';
  assert.equal(formatFromEarth(384400e3), `384${nn}400${nb}km ·${nb}light${nb}1.28${nb}s`);
  // 1 AU is 499.005 light-seconds: 8 min 19 s.
  assert.equal(formatFromEarth(149597870700), `1.0000${nb}AU ·${nb}light${nb}8${nb}min${nb}19${nb}s`);
  assert.equal(formatFromEarth(5 * 149597870700), `5.0000${nb}AU ·${nb}light${nb}41${nb}min${nb}35${nb}s`);
  assert.equal(formatFromEarth(30 * 149597870700), `30.0000${nb}AU ·${nb}light${nb}4${nb}h${nb}10${nb}min`);
  // Totals are rounded before splitting: 3599.7 s is 1 h 0 min, never 59 min 60 s.
  assert.equal(formatFromEarth(3599.7 * 299792458), `7.2138${nb}AU ·${nb}light${nb}1${nb}h${nb}0${nb}min`);
  assert.equal(formatFromEarth(59.999 * 299792458), `0.1202${nb}AU ·${nb}light${nb}1${nb}min${nb}0${nb}s`);
});

test('a linked instant reads ?at= in TDB and writes it back to the minute (SPEC A141)', () => {
  assert.equal(sceneSecondsAtLink('2026-06-10'), 86400);
  assert.equal(sceneSecondsAtLink('2026-06-10T06:30'), 86400 + 6.5 * 3600);
  for (const bad of [null, '', '2026-6-10', '2026-06-10T6:30', '2026-06-10 06:30', '2026-06-08', '2080-01-01', '2026-06-10T06:30Z'])
    assert.ok(Number.isNaN(sceneSecondsAtLink(bad)), String(bad));
  assert.equal(linkForSceneSeconds(86400 + 6.5 * 3600 + 42), '2026-06-10T06:30');
  assert.equal(sceneSecondsAtLink(linkForSceneSeconds(123456789)), Math.floor(123456789 / 60) * 60);
});
