import assert from 'node:assert/strict';
import test from 'node:test';
import { readdir, readFile } from 'node:fs/promises';
import { coreBodyCount, jovianMoonCount, smallBodyCatalogCount, formatCount } from '../src/lib/siteCounts.ts';
import { implementedBodies } from '../src/lib/bodies.ts';
import manifest from '../public/catalog/manifest.json' with { type: 'json' };

test('page copy derives scene and catalog counts from data instead of literals', async () => {
  assert.equal(coreBodyCount, implementedBodies.length);
  assert.equal(jovianMoonCount, implementedBodies.filter(body => body.parent === 'Jupiter').length);
  assert.equal(smallBodyCatalogCount, manifest.count);
  const literals = [String(coreBodyCount), formatCount(smallBodyCatalogCount), String(smallBodyCatalogCount), `${jovianMoonCount}-moon`];
  const pages = (await readdir(new URL('../src/', import.meta.url), { recursive: true })).filter(file => file.endsWith('.astro'));
  assert.ok(pages.length > 10);
  for (const file of pages) {
    const source = await readFile(new URL(`../src/${file}`, import.meta.url), 'utf8');
    for (const literal of literals) assert.ok(!new RegExp(`(^|[^0-9.,])${literal.replace(/[,]/g, ',')}([^0-9,]|$)`).test(source), `${file} hard-codes ${literal}`);
  }
});
