import assert from 'node:assert/strict';
import { readFileSync, readdirSync, statSync } from 'node:fs';
import { join } from 'node:path';
import test from 'node:test';

const root = new URL('..', import.meta.url).pathname;

function files(dir) {
  return readdirSync(dir).flatMap(name => {
    const path = join(dir, name);
    return statSync(path).isDirectory() ? files(path) : [path];
  });
}

// An undefined custom property silently computes to its initial value: the
// redesign once left page styles pointing at retired tokens, which made text
// cream-on-cream and removed keyboard focus outlines.
test('every CSS custom property the site uses is defined in global.css', () => {
  const global = readFileSync(join(root, 'public/styles/global.css'), 'utf8');
  const defined = new Set([...global.matchAll(/(--[a-z0-9-]+)\s*:/g)].map(match => match[1]));
  const sources = [join(root, 'public/styles/global.css'), ...files(join(root, 'src')).filter(path => /\.(astro|css|ts)$/.test(path))];
  const missing = sources.flatMap(path => [...readFileSync(path, 'utf8').matchAll(/var\((--[a-z0-9-]+)\s*[,)]/g)]
    .filter(match => !defined.has(match[1]))
    .map(match => `${path.slice(root.length)}: ${match[1]}`));
  assert.deepEqual(missing, []);
});
