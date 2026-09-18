import { test, expect } from '@playwright/test';

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
  await page.getByRole('searchbox', { name: 'Name, designation, or SPK ID' }).fill('Ceres');
  await page.getByRole('button', { name: 'Search catalog', exact: true }).click();
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
});
