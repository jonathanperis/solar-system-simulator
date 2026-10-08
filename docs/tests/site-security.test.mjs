import assert from 'node:assert/strict';
import test from 'node:test';
import { readFile } from 'node:fs/promises';
import { contentSecurityPolicy, analyticsHosts } from '../src/lib/csp.ts';

const directives = policy => Object.fromEntries(policy.split(';').map(part => part.trim().split(/\s+/)).map(([name, ...sources]) => [name, sources]));

test('analytics-free builds allow only same-origin code plus WebAssembly compilation', () => {
  const policy = directives(contentSecurityPolicy(false));
  assert.deepEqual(policy['script-src'], ["'self'", "'wasm-unsafe-eval'"]);
  assert.deepEqual(policy['style-src'], ["'self'"]);
  assert.deepEqual(policy['font-src'], ["'self'"]);
  assert.deepEqual(policy['connect-src'], ["'self'"]);
  assert.deepEqual(policy['object-src'], ["'none'"]);
  assert.ok(!contentSecurityPolicy(false).includes('google'));
  assert.ok(!/unsafe-inline|unsafe-eval'|\*/.test(contentSecurityPolicy(false).replace("'wasm-unsafe-eval'", '')));
});

test('V17 analytics builds add only the Google tag hosts', () => {
  const policy = directives(contentSecurityPolicy(true));
  assert.deepEqual(policy['script-src'], ["'self'", "'wasm-unsafe-eval'", ...analyticsHosts.script]);
  assert.deepEqual(policy['connect-src'], ["'self'", ...analyticsHosts.connect]);
  assert.deepEqual(policy['img-src'], ["'self'", ...analyticsHosts.img]);
  assert.deepEqual(policy['style-src'], ["'self'"]);
});

test('analytics bootstrap is a static file that validates the measurement ID', async () => {
  const source = await readFile(new URL('../public/scripts/analytics.js', import.meta.url), 'utf8');
  assert.match(source, /\^G-\[A-Z0-9\]\+\$/);
  assert.ok(source.includes('https://www.googletagmanager.com/gtag/js?id='));
});
