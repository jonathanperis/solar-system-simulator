import { test, expect } from './fixtures';

const base = '/solar-system-simulator/';

test('phone simulator keeps every panel clear of the view, with object handoffs, filtering and dialog focus', async ({ page }) => {
  await page.setViewportSize({ width: 390, height: 844 });
  await page.goto(`${base}?body=earth`);
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Earth');
  // On a phone the panels stack around the canvas instead of covering it.
  const canvas = (await page.locator('#canvas').boundingBox())!;
  const inspector = (await page.locator('.inspector').boundingBox())!;
  const dock = (await page.locator('.dock').boundingBox())!;
  expect(canvas.height).toBeGreaterThan(250);
  expect(canvas.y + canvas.height).toBeLessThanOrEqual(inspector.y + 1);
  expect(inspector.y + inspector.height).toBeLessThanOrEqual(dock.y + 1);
  expect(dock.y + dock.height).toBeLessThanOrEqual(844);
  await page.getByRole('button', { name: 'Pause', exact: true }).click();
  await page.getByRole('button', { name: 'Find', exact: true }).click();
  const dialog = page.getByRole('dialog', { name: 'Find a body', exact: true });
  await expect(dialog).toBeVisible();
  await dialog.getByRole('searchbox', { name: 'Name' }).fill('not-a-real-body');
  await expect(page.locator('[data-body-filter-status]')).toHaveText('No matching bodies. Earth remains selected.');
  await dialog.getByRole('button', { name: 'Clear', exact: true }).click();
  await dialog.getByRole('combobox', { name: 'Body', exact: true }).selectOption({ label: 'Moon — Earth and Mars moons' });
  await page.keyboard.press('Escape');
  await expect(page.getByRole('button', { name: 'Find', exact: true })).toBeFocused();
  await page.getByRole('button', { name: 'Whole scene', exact: true }).click();
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Sun');
  await page.getByRole('button', { name: 'Restart', exact: true }).click();
  await expect(page.locator('[data-runtime-elapsed]')).toHaveText('2026-06-09 00:00 TDB');
  await expect(page.getByRole('button', { name: 'Resume', exact: true })).toBeVisible();
  // The data sheet fits a narrow phone and its close button stays inside it.
  await page.setViewportSize({ width: 320, height: 844 });
  await page.getByRole('button', { name: 'Data', exact: true }).click();
  const sheet = (await page.getByRole('dialog', { name: 'Data' }).boundingBox())!;
  const close = page.getByRole('button', { name: 'Close data' });
  const closeBox = (await close.boundingBox())!;
  expect(closeBox.y).toBeGreaterThanOrEqual(sheet.y);
  expect(closeBox.y + closeBox.height).toBeLessThanOrEqual(sheet.y + sheet.height);
  await close.click();
  await expect(page.getByRole('button', { name: 'Data', exact: true })).toBeFocused();
});

test('instrument panels never cover each other, from short phones to desktops, during a lesson', async ({ page }) => {
  const overlap = (a: { x: number; y: number; width: number; height: number }, b: typeof a) =>
    Math.min(a.x + a.width, b.x + b.width) - Math.max(a.x, b.x) > 1 && Math.min(a.y + a.height, b.y + b.height) - Math.max(a.y, b.y) > 1;
  await page.goto(`${base}?lesson=collision`);
  await expect(page.locator('[data-lesson-strip]')).toBeVisible({ timeout: 30000 });
  for (const [width, height] of [[375, 548], [844, 390], [820, 1180], [1024, 768], [1280, 800], [1440, 900]]) {
    await page.setViewportSize({ width, height });
    // A long status line must wrap inside the strip, not widen it.
    await page.getByRole('button', { name: 'Load', exact: true }).click();
    await expect(page.locator('[data-lesson-status]')).not.toBeEmpty();
    const panels = await Promise.all(['.lesson-strip', '.inspector', '.dock'].map(selector => page.locator(selector).boundingBox()));
    for (let i = 0; i < panels.length; i++) for (let j = i + 1; j < panels.length; j++)
      expect(overlap(panels[i]!, panels[j]!), `panels ${i}/${j} overlap at ${width}x${height}`).toBe(false);
    // Every dock control stays reachable: in view or reachable by scrolling the instrument.
    const pause = page.getByRole('button', { name: /^(Pause|Resume)$/ });
    await pause.scrollIntoViewIfNeeded();
    await expect(pause).toBeInViewport();
    expect((await page.locator('#canvas').boundingBox())!.height, `canvas at ${width}x${height}`).toBeGreaterThanOrEqual(180);
  }
});

test('the instrument stays lean and every page keeps readable text', async ({ page }) => {
  await page.setViewportSize({ width: 1440, height: 900 });
  await page.goto(base);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  // DESIGN.md: the simulator shows controls and readouts, not prose.
  const words = await page.evaluate(() => document.body.innerText.split(/\s+/).filter(Boolean).length);
  expect(words).toBeLessThan(150);
  for (const path of ['', 'learn/', 'catalog/', 'about/']) {
    for (const width of [320, 1280]) {
      await page.setViewportSize({ width, height: 800 });
      await page.goto(`${base}${path}`);
      const smallest = await page.evaluate(() => Math.min(...[...document.querySelectorAll('body *')]
        .filter(element => element.checkVisibility() && [...element.childNodes].some(node => node.nodeType === 3 && node.textContent!.trim()))
        .map(element => parseFloat(getComputedStyle(element).fontSize))));
      expect(smallest, `smallest visible text on /${path} at ${width}px`).toBeGreaterThanOrEqual(12);
      expect(await page.evaluate(() => document.documentElement.scrollWidth), `/${path} at ${width}px scrolls sideways`).toBeLessThanOrEqual(width);
    }
  }
});

test('the simulator canvas exposes an application role with keyboard help and text readouts', async ({ page }) => {
  await page.goto(base);
  const canvas = page.getByRole('application', { name: 'Live solar system simulation' });
  await expect(canvas).toHaveAttribute('aria-describedby', /canvas-keys/);
  await expect(page.locator('#canvas-keys')).toContainText('Tab leaves the view');
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body:');
});

test('catalog search hands a body to the simulator, and Learn links open their lesson', async ({ page }) => {
  await page.goto(`${base}catalog/`);
  await page.getByRole('searchbox', { name: 'Search' }).fill('earth');
  await expect(page.locator('[data-body-count]')).toHaveText('1 body.');
  await page.getByRole('link', { name: 'Earth', exact: true }).click();
  await expect(page).toHaveURL(new RegExp(`${base}\\?body=earth$`));
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Earth');
  await page.goto(`${base}learn/`);
  await page.locator(`a[href="${base}?lesson=dart"]`).click();
  await expect(page.locator('[data-active-scene]')).toContainText('2 active bodies · DART impact');
  await expect(page.locator('[data-lesson-strip]')).toBeVisible();
  await expect(page.getByRole('combobox', { name: 'Lesson', exact: true })).toHaveValue(/\d+/);
  await expect(page.locator('[data-inspector-period]')).toContainText('hours');
});

test('comparison starts with a question and points invalid spacing to its field', async ({ page }) => {
  await page.goto(`${base}learn/compare/`);
  await expect(page.locator('[data-lab-status]')).toContainText('Comparison lab ready');
  await expect(page.getByRole('heading', { name: 'Does a smaller timestep improve an orbit?', exact: true })).toBeVisible();
  await page.getByRole('button', { name: 'Start comparison', exact: true }).click();
  await expect(page.locator('[data-lab-status]')).toHaveText('Comparison complete.');
  await page.getByText('Adjust experiment settings', { exact: true }).click();
  await page.getByLabel('Sample spacing (seconds)').fill('3500');
  await page.getByText('Adjust experiment settings', { exact: true }).click();
  await page.getByRole('button', { name: 'Start comparison', exact: true }).click();
  await expect(page.getByLabel('Sample spacing (seconds)')).toBeFocused();
  await expect(page.locator('[data-config-error]')).toContainText('Try 3600 seconds');
  await expect(page.locator('[data-lab-status]')).toContainText('Previous run retained and paused');
  await expect(page.locator('[data-matched-time]')).toHaveText('86400 s');
});

test('mobile small-body details are visible and return to the refreshed result', async ({ page }) => {
  test.setTimeout(120000);
  await page.setViewportSize({ width: 390, height: 844 });
  await page.goto(`${base}catalog/small-bodies/`);
  const indexRequests: string[] = [];
  page.on('request', request => { if (/-index\.json\.gz/.test(request.url())) indexRequests.push(request.url()); });
  await expect(page.locator('[data-search-status]')).toContainText('matches');
  indexRequests.length = 0;
  await page.getByRole('searchbox', { name: 'Name, designation, or SPK ID' }).fill('Ceres');
  await page.getByRole('button', { name: 'Search catalog', exact: true }).click();
  // The full-index download is stated and confirmed before any byte is fetched.
  const confirm = page.getByRole('button', { name: 'Download index and search', exact: true });
  await expect(confirm).toBeFocused();
  await expect(page.locator('[data-scan-consent]')).toContainText(/about \d+ MB compressed/);
  expect(indexRequests).toEqual([]);
  await confirm.click();
  const result = page.getByRole('button', { name: '1 Ceres (A801 AA)', exact: true });
  await result.click({ timeout: 60000 });
  await expect(result).toHaveAttribute('aria-haspopup', 'dialog');
  await expect(result).not.toHaveAttribute('aria-pressed');
  await expect(page.getByRole('dialog', { name: 'Object details' })).toBeVisible();
  await expect(page.locator('[data-object-title]')).toHaveText('1 Ceres (A801 AA)');
  await expect(page.getByRole('button', { name: 'Add to experiment', exact: true })).toBeEnabled();
  await page.getByRole('button', { name: 'Add to experiment', exact: true }).click();
  await page.keyboard.press('Escape');
  await expect(result).toBeFocused();
  await expect(page.locator('[data-basket-summary]')).toContainText('1 / 16');
  // Later searches and filters reuse the in-memory index: no further index downloads.
  const downloaded = indexRequests.length;
  await page.getByRole('searchbox', { name: 'Name, designation, or SPK ID' }).fill('Pluto');
  await page.getByRole('button', { name: 'Search catalog', exact: true }).click();
  await expect(page.getByRole('button', { name: '134340 Pluto (1930 BM)', exact: true })).toBeVisible();
  await page.getByLabel('Physical size').selectOption('known');
  await page.getByRole('button', { name: 'Search catalog', exact: true }).click();
  await expect(page.locator('[data-search-status]')).toContainText('1 match;');
  expect(indexRequests.length).toBe(downloaded);
});
