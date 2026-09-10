import assert from 'node:assert/strict';
import test from 'node:test';
import { createSimulatorModule, preserveBrowserTabNavigation } from '../src/lib/simulator.ts';

test('web canvas leaves Tab available for browser navigation', () => {
  const stopped = [];
  preserveBrowserTabNavigation({ key: 'Tab', stopImmediatePropagation: () => stopped.push('Tab') });
  preserveBrowserTabNavigation({ key: 'C', stopImmediatePropagation: () => stopped.push('C') });
  assert.deepEqual(stopped, ['Tab']);
});

test('runtime pairs asset revisions, reports state, and preserves failures after late callbacks', () => {
  const readouts = Object.fromEntries(['status', 'controls', 'elapsed', 'interval'].map(key => [key, { textContent: '' }]));
  const runtime = createSimulatorModule({}, readouts,
    new URL('https://example.test/solar-system-simulator/wasm/solar-system-simulator.js?revision=abc123'));
  assert.equal(runtime.locateFile('solar-system-simulator.wasm'),
    'https://example.test/solar-system-simulator/wasm/solar-system-simulator.wasm?revision=abc123');
  runtime.setStatus('Downloading simulator…');
  assert.equal(readouts.status.textContent, 'Downloading simulator…');
  runtime.reportState('Sun', 'Illustrative', 0, 300, 0);
  assert.equal(readouts.status.textContent, 'Running physics simulation');
  runtime.reportState('Mercury', 'Real scale', 86.83, 9600, 0);
  assert.equal(readouts.controls.textContent, 'Focused body: Mercury; view: Real scale.');
  assert.equal(readouts.elapsed.textContent, '86.83 simulated days');
  assert.equal(readouts.interval.textContent, '2.67 simulated hours');

  runtime.onAbort('Unable to load WebAssembly');
  runtime.setStatus('');
  runtime.reportState('Sun', 'Illustrative', 0, 300, 0);
  assert.equal(readouts.status.textContent, 'Runtime error: Unable to load WebAssembly');
});
