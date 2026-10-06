import { test, expect } from './fixtures';

const base = '/solar-system-simulator/';

test('scene-first mobile exploration preserves object handoffs, filtering, and dialog focus', async ({ page }) => {
  await page.setViewportSize({ width: 390, height: 844 });
  await page.goto(`${base}simulator/?body=earth`);
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Earth');
  const canvas = await page.locator('#canvas').boundingBox();
  const pause = page.getByRole('button', { name: 'Pause', exact: true });
  const playback = await pause.boundingBox();
  expect(canvas!.y).toBeLessThan(400);
  expect(playback!.y + playback!.height).toBeLessThan(844);
  await pause.click();
  await page.getByRole('button', { name: 'Find an object', exact: true }).click();
  const dialog = page.getByRole('dialog', { name: 'Find an object', exact: true });
  await expect(dialog).toBeVisible();
  await page.getByRole('searchbox', { name: 'Find a body' }).fill('not-a-real-body');
  await expect(page.locator('[data-body-filter-status]')).toHaveText('No matching bodies. Earth remains selected.');
  await page.getByRole('button', { name: 'Clear filters' }).click();
  await page.getByRole('combobox', { name: 'Selected body', exact: true }).selectOption({ label: 'Moon — Earth and Mars moons' });
  await page.keyboard.press('Escape');
  await expect(page.getByRole('button', { name: 'Find an object', exact: true })).toBeFocused();
  await page.getByRole('button', { name: 'Show whole system', exact: true }).click();
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Sun');
  await page.getByRole('button', { name: 'Restart', exact: true }).click();
  await expect(page.locator('[data-runtime-elapsed]')).toContainText('0 s');
  await expect(page.getByRole('button', { name: 'Resume', exact: true })).toBeVisible();
  await page.setViewportSize({ width: 320, height: 844 });
  await page.getByRole('button', { name: 'Learn', exact: true }).click();
  await page.getByText('Advanced physics settings', { exact: true }).click();
  await page.getByRole('link', { name: 'Compare two experiments →', exact: true }).scrollIntoViewIfNeeded();
  const sheet = await page.getByRole('dialog', { name: 'Choose an activity' }).boundingBox();
  const close = page.getByRole('button', { name: 'Close learning activities' });
  const closeBox = await close.boundingBox();
  expect(closeBox!.y).toBeGreaterThanOrEqual(sheet!.y);
  expect(closeBox!.y + closeBox!.height).toBeLessThanOrEqual(sheet!.y + sheet!.height);
  await close.click();
  await expect(page.getByRole('button', { name: 'Learn', exact: true })).toBeFocused();
});

test('atlas announces through one status region, settles drags, and keeps readable text', async ({ page }) => {
  await page.emulateMedia({ reducedMotion: 'reduce' });
  await page.setViewportSize({ width: 1280, height: 800 });
  await page.goto(base);
  const atlas = page.locator('[data-orbital-atlas]');
  await expect(atlas.locator('[aria-live], [role="status"]')).toHaveCount(1);
  await expect(atlas.locator('[data-atlas-body][aria-expanded]')).toHaveCount(0);
  await expect(atlas.locator('[data-atlas-body]').first()).toHaveAttribute('aria-pressed', /true|false/);
  expect(await atlas.locator('.atlas-body').first().evaluate(element => getComputedStyle(element).transitionDuration)).toBe('0s');
  // Record every status change during a pointer sweep across the chart.
  await page.evaluate(() => {
    const status = document.querySelector('[data-atlas-status]')!;
    (window as unknown as { announcements: string[] }).announcements = [];
    new MutationObserver(() => (window as unknown as { announcements: string[] }).announcements.push(status.textContent ?? ''))
      .observe(status, { childList: true, characterData: true, subtree: true });
  });
  const box = (await page.locator('[data-atlas-plate="heliocentric"]').boundingBox())!;
  const cx = box.x + box.width / 2, cy = box.y + box.height / 2, r = box.width * 0.42;
  await page.mouse.move(cx + r, cy);
  await page.mouse.down();
  for (let step = 1; step <= 24; ++step) await page.mouse.move(cx + r * Math.cos(step * Math.PI / 12), cy + r * Math.sin(step * Math.PI / 12));
  expect(await page.evaluate(() => (window as unknown as { announcements: string[] }).announcements.length)).toBe(0);
  await page.mouse.up();
  const announcements = await page.evaluate(() => (window as unknown as { announcements: string[] }).announcements);
  expect(announcements).toHaveLength(1);
  expect(announcements[0]).toMatch(/selected\./);
  for (const width of [320, 1280]) {
    await page.setViewportSize({ width, height: 800 });
    const smallest = await page.evaluate(() => Math.min(...[...document.querySelectorAll('body *')]
      .filter(element => element.checkVisibility() && [...element.childNodes].some(node => node.nodeType === 3 && node.textContent!.trim()))
      .map(element => parseFloat(getComputedStyle(element).fontSize))));
    expect(smallest, `smallest visible text at ${width}px`).toBeGreaterThanOrEqual(12);
  }
});

test('the simulator canvas exposes an application role with keyboard help and text readouts', async ({ page }) => {
  await page.goto(`${base}simulator/`);
  const canvas = page.getByRole('application', { name: 'Live solar system simulation' });
  await expect(canvas).toHaveAttribute('aria-describedby', /canvas-keys/);
  await expect(page.locator('#canvas-keys')).toContainText('Tab leaves the view');
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body:');
});

test('core catalog discovery, object handoff, and task-word help stay connected', async ({ page }) => {
  await page.goto(`${base}body-catalog/`);
  await page.getByRole('searchbox', { name: 'Search bodies' }).fill('earth');
  await expect(page.locator('[data-body-count]')).toHaveText('1 body found.');
  await page.getByRole('link', { name: 'Explore Earth', exact: true }).click();
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Earth');
  await page.goto(`${base}docs/`);
  await page.getByRole('searchbox', { name: 'Find a help page' }).fill('pause');
  await expect(page.locator('[data-doc-count]')).toHaveText('1 matching page');
  await expect(page.locator('.manual-index-row:visible')).toContainText('Controls');
});

test('comparison starts with a question and points invalid spacing to its field', async ({ page }) => {
  await page.goto(`${base}compare/`);
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
  await page.goto(`${base}small-bodies/`);
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
