import { test, expect, type Page } from '@playwright/test';

const base = '/solar-system-simulator/';

/** Record CSP violations from both the DOM event and the console report. */
async function watchPolicy(page: Page): Promise<string[]> {
  const violations: string[] = [];
  page.on('console', message => {
    if (/Content Security Policy|Refused to/i.test(message.text())) violations.push(message.text());
  });
  await page.addInitScript(() => {
    document.addEventListener('securitypolicyviolation', event => {
      console.error(`Refused to load: CSP ${event.violatedDirective} blocked ${event.blockedURI}`);
    });
  });
  return violations;
}

test('every page runs under its CSP with self-hosted fonts and no third-party requests', async ({ page }) => {
  const violations = await watchPolicy(page);
  const external: string[] = [];
  page.on('request', request => {
    const url = new URL(request.url());
    if (!['127.0.0.1', 'localhost'].includes(url.hostname) && url.protocol.startsWith('http')) external.push(request.url());
  });
  for (const route of ['', 'docs/', 'docs/build-and-web/', 'physics/', 'body-catalog/', 'source-atlas/', 'pipeline/']) {
    await page.goto(`${base}${route}`);
    await expect(page.locator('meta[http-equiv="Content-Security-Policy"]')).toHaveCount(1);
    await expect(page.locator('meta[name="generator"]')).toHaveAttribute('content', /^Astro v/);
  }
  await page.goto(base);
  expect(await page.evaluate(async () => {
    await document.fonts.ready;
    return ['400 1rem "Cormorant Garamond"', 'italic 400 1rem "Cormorant Garamond"', '700 1rem "JetBrains Mono"'].map(font => document.fonts.check(font));
  })).toEqual([true, true, true]);
  await page.goto(`${base}simulator/`);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running physics simulation');
  await page.goto(`${base}compare/`);
  await expect(page.locator('[data-lab-status]')).toContainText('Comparison lab ready');
  await page.getByRole('button', { name: 'Start comparison', exact: true }).click();
  await expect(page.getByRole('status')).toHaveText('Comparison complete.');
  await page.getByText('Save, share, or download', { exact: true }).click();
  const download = page.waitForEvent('download');
  await page.getByRole('button', { name: 'Export both runs (CSV)' }).click();
  expect((await download).suggestedFilename()).toMatch(/\.csv$/);
  await page.goto(`${base}small-bodies/`);
  await page.getByRole('searchbox', { name: 'Name, designation, or SPK ID' }).fill('433');
  await page.getByRole('button', { name: 'Search catalog', exact: true }).click();
  await expect(page.getByRole('button', { name: '433 Eros (A898 PA)', exact: true })).toBeVisible({ timeout: 30000 });
  expect(violations).toEqual([]);
  expect(external).toEqual([]);
});

test('oversized or malformed session experiments are rejected visibly before reaching C', async ({ page }) => {
  await page.goto(`${base}simulator/`);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running physics simulation');
  for (const [stored, message] of [
    [JSON.stringify({ text: `SOLAR_EXPERIMENT_V1 2461200.5\n${'x'.repeat(20000)}`, snapshot: 'abc', count: 1 }), 'too large'],
    ['{"text":', 'could not be read']
  ]) {
    await page.evaluate(value => sessionStorage.setItem('solar-catalog-experiment', value), stored);
    await page.goto(`${base}simulator/?experiment=1`);
    await expect(page.locator('[data-experiment-status]')).toContainText(message);
    await expect(page.locator('[data-experiment-status]')).not.toContainText('Error:');
    await page.getByRole('button', { name: 'Start prepared experiment' }).click();
    await expect(page.locator('[data-experiment-status]')).toContainText('Choose bodies in the small-body atlas first.');
    await expect(page.locator('[data-runtime-status]')).not.toContainText('Runtime error');
  }
});

test('without Web Crypto the catalog explains the HTTPS requirement instead of skipping integrity checks', async ({ page }) => {
  // Plain-HTTP LAN previews expose no crypto.subtle; emulate that here.
  await page.addInitScript(() => Object.defineProperty(Crypto.prototype, 'subtle', { get: () => undefined }));
  const shards: string[] = [];
  page.on('request', request => { if (request.url().includes('.json.gz')) shards.push(request.url()); });
  await page.goto(`${base}small-bodies/`);
  await expect(page.locator('[data-search-status]')).toContainText('HTTPS or http://localhost');
  await expect(page.getByRole('button', { name: 'Search catalog', exact: true })).toBeDisabled();
  expect(shards).toEqual([]);
});

test('the historic runtime URL is an Astro page that forwards to the simulator', async ({ page }) => {
  const violations = await watchPolicy(page);
  await page.goto(`${base}wasm/solar-system-simulator.html`);
  await expect(page).toHaveURL(new RegExp(`${base}simulator/$`));
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running physics simulation');
  expect(violations).toEqual([]);
});
