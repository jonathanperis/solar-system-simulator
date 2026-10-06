import { readFileSync } from 'node:fs';
import { test, expect } from './fixtures';

const base = '/solar-system-simulator/';
type Shard = { index: { file: string }; minId: number; maxId: number };
const manifest = JSON.parse(readFileSync(new URL('../public/catalog/manifest.json', import.meta.url), 'utf8')) as { shards: Shard[] };
const order = new Map(manifest.shards.map((shard, index) => [shard.index.file, index]));
// Index shards that an identity look-up of asteroid 433 reads.
const identityFiles = new Set(manifest.shards.filter(shard => shard.minId <= 20000433 && shard.maxId >= 20000433).map(shard => shard.index.file));

test('full-index journey: consent, superseding search mid-download, Stop, restart and in-memory reuse', async ({ page }) => {
  test.setTimeout(180000);
  // Hold the full scan partway (but never the identity shards) so Stop and the
  // superseding search happen deterministically while the download is active.
  let holding = true, release = () => {};
  const gate = new Promise<void>(resolve => { release = resolve; });
  const indexRequests: string[] = [];
  await page.route(/\/catalog\/[A-Z]{3}-\d{4}-index\.json\.gz/, async route => {
    const file = new URL(route.request().url()).pathname.split('/').at(-1)!;
    indexRequests.push(file);
    if (holding && (order.get(file) ?? 0) >= 20 && !identityFiles.has(file)) await gate;
    try { await route.continue(); } catch { /* the request was aborted by Stop */ }
  });
  await page.goto(`${base}small-bodies/`);
  await expect(page.locator('[data-search-status]')).toContainText('matches');
  await page.evaluate(() => {
    const region = document.querySelector('[data-scan-announce]')!;
    (window as unknown as { spoken: string[] }).spoken = [];
    new MutationObserver(() => (window as unknown as { spoken: string[] }).spoken.push(region.textContent ?? ''))
      .observe(region, { childList: true, characterData: true, subtree: true });
  });
  const search = async (query: string) => {
    await page.getByRole('searchbox', { name: 'Name, designation, or SPK ID' }).fill(query);
    await page.getByRole('button', { name: 'Search catalog', exact: true }).click();
  };
  const progress = page.locator('[data-scan-progress]');
  const stop = page.getByRole('button', { name: 'Stop index download', exact: true });

  await search('Vesta');
  await page.getByRole('button', { name: 'Download index and search', exact: true }).click();
  await expect(progress).toBeVisible();
  await expect(page.locator('[data-scan-progress-text]')).toContainText(`of ${manifest.shards.length} files`);
  await expect(page.locator('[data-search-status]')).toContainText('Waiting for the catalog index download');

  // A number search mid-download answers from its own shards; the download keeps running visibly.
  await search('433');
  await expect(page.getByRole('button', { name: '433 Eros (A898 PA)', exact: true })).toBeVisible();
  await expect(progress).toBeVisible();
  await expect(stop).toBeVisible();

  await stop.click();
  await expect(progress).toBeHidden();
  await expect(page.locator('[data-scan-announce]')).toHaveText('Catalog index download stopped.');
  holding = false; release();

  // After Stop, a name search asks again before downloading.
  await search('Vesta');
  await expect(page.getByRole('button', { name: 'Download index and search', exact: true })).toBeFocused();
  await page.getByRole('button', { name: 'Download index and search', exact: true }).click();
  await expect(page.getByRole('button', { name: '4 Vesta (A807 FA)', exact: true })).toBeVisible({ timeout: 120000 });
  await expect(progress).toBeHidden();
  await expect(page.locator('[data-scan-announce]')).toHaveText('Catalog index ready; searches now run from memory.');
  const spoken = await page.evaluate(() => (window as unknown as { spoken: string[] }).spoken);
  expect(spoken.length, spoken.join(' | ')).toBeLessThanOrEqual(16);

  const downloaded = indexRequests.length;
  await search('Pallas');
  await expect(page.getByRole('button', { name: '2 Pallas (A802 FA)', exact: true })).toBeVisible();
  await page.getByLabel('Physical size').selectOption('known');
  await page.getByRole('button', { name: 'Search catalog', exact: true }).click();
  await expect(page.locator('[data-search-status]')).toHaveText(/^\d+ match(es)?; /);
  await expect(page.getByRole('button', { name: '2 Pallas (A802 FA)', exact: true })).toBeVisible();
  expect(indexRequests.length).toBe(downloaded);
});
