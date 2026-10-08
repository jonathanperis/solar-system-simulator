import type { Page } from '@playwright/test';
import { test, expect, analyticsUrl } from './fixtures';

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

test('every page runs under its CSP with self-hosted fonts and no third-party requests', async ({ page, analyticsRequests }) => {
  const violations = await watchPolicy(page);
  const external: string[] = [];
  page.on('request', request => {
    const url = new URL(request.url());
    // Analytics hosts are stubbed by the fixture and asserted separately below.
    if (!['127.0.0.1', 'localhost'].includes(url.hostname) && url.protocol.startsWith('http') && !analyticsUrl.test(request.url()))
      external.push(request.url());
  });
  for (const route of ['learn/', 'catalog/', 'about/']) {
    await page.goto(`${base}${route}`);
    await expect(page.locator('meta[http-equiv="Content-Security-Policy"]')).toHaveCount(1);
    await expect(page.locator('meta[name="generator"]')).toHaveAttribute('content', /^Astro v/);
  }
  // The notebook pages load both self-hosted fonts; the instrument only the mono.
  await page.goto(`${base}about/`);
  expect(await page.evaluate(async () => {
    await document.fonts.ready;
    return ['600 1rem "Cormorant Garamond"', '700 1rem "JetBrains Mono"'].map(font => document.fonts.check(font));
  })).toEqual([true, true]);
  await page.goto(base);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  await page.goto(`${base}learn/compare/`);
  await expect(page.locator('[data-lab-status]')).toContainText('Comparison lab ready');
  await page.getByRole('button', { name: 'Start comparison', exact: true }).click();
  await expect(page.getByRole('status')).toHaveText('Comparison complete.');
  await page.getByText('Save, share, or download', { exact: true }).click();
  const download = page.waitForEvent('download');
  await page.getByRole('button', { name: 'Export both runs (CSV)' }).click();
  expect((await download).suggestedFilename()).toMatch(/\.csv$/);
  await page.goto(`${base}catalog/small-bodies/`);
  await page.getByRole('searchbox', { name: 'Name, designation, or SPK ID' }).fill('433');
  await page.getByRole('button', { name: 'Search catalog', exact: true }).click();
  await expect(page.getByRole('button', { name: '433 Eros (A898 PA)', exact: true })).toBeVisible({ timeout: 30000 });
  expect(violations).toEqual([]);
  expect(external).toEqual([]);
  // V17: only builds carrying PUBLIC_GA_ID load the analytics bootstrap, and
  // then the CSP must admit its loader; other builds contact nobody at all.
  const analyticsBuild = await page.locator('script[data-ga-id]').count() > 0;
  if (analyticsBuild) expect(analyticsRequests.some(url => url.startsWith('https://www.googletagmanager.com/gtag/js?id='))).toBe(true);
  else expect(analyticsRequests).toEqual([]);
});

test('oversized or malformed session experiments are rejected visibly before reaching C', async ({ page }) => {
  await page.goto(base);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  for (const [stored, message] of [
    [JSON.stringify({ text: `SOLAR_EXPERIMENT_V1 2461200.5\n${'x'.repeat(20000)}`, snapshot: 'abc', count: 1 }), 'too large'],
    ['{"text":', 'could not be read']
  ]) {
    await page.evaluate(value => sessionStorage.setItem('solar-catalog-experiment', value), stored);
    await page.goto(`${base}?experiment=1`);
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
  await page.goto(`${base}catalog/small-bodies/`);
  await expect(page.locator('[data-search-status]')).toContainText('HTTPS or http://localhost');
  await expect(page.getByRole('button', { name: 'Search catalog', exact: true })).toBeDisabled();
  expect(shards).toEqual([]);
});

test('the 404 page is an Astro page on the shared layout with working base-absolute links', async ({ page }) => {
  const violations = await watchPolicy(page);
  await page.goto(`${base}404.html`);
  await expect(page.locator('meta[name="generator"]')).toHaveAttribute('content', /^Astro v/);
  await expect(page.locator('meta[name="robots"]')).toHaveAttribute('content', 'noindex');
  await expect(page.locator('link[rel="canonical"]')).toHaveCount(0);
  await expect(page.getByRole('heading', { name: 'Not found.' })).toBeVisible();
  await page.getByRole('main').getByRole('link', { name: 'Catalog', exact: true }).click();
  await expect(page).toHaveURL(new RegExp(`${base}catalog/$`));
  expect(violations).toEqual([]);
});

test('the historic runtime URL is an Astro page that forwards to the simulator', async ({ page }) => {
  const violations = await watchPolicy(page);
  await page.goto(`${base}wasm/solar-system-simulator.html`);
  await expect(page).toHaveURL(new RegExp(`${base}$`));
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  expect(violations).toEqual([]);
});

// SPEC A99: retired URLs forward to their new home and keep the query and
// fragment, so shared ?body= links and section anchors still land.
test('retired URLs forward with their query and fragment intact', async ({ page }) => {
  const violations = await watchPolicy(page);
  await page.goto(`${base}simulator/?body=titan`);
  await expect(page).toHaveURL(new RegExp(`${base}\\?body=titan$`));
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Titan;');
  await page.goto(`${base}body-catalog/#phobos`);
  await expect(page).toHaveURL(new RegExp(`${base}catalog/#phobos$`));
  await page.goto(`${base}physics/`);
  await expect(page).toHaveURL(new RegExp(`${base}about/#model$`));
  await expect(page.locator('#model')).toBeInViewport();
  await page.goto(`${base}docs/controls/`);
  await expect(page).toHaveURL(new RegExp(`${base}about/#controls$`));
  expect(violations).toEqual([]);
});
