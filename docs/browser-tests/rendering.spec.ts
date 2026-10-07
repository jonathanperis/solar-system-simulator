import { test, expect } from './fixtures';

const base = '/solar-system-simulator/';

// SPEC A64-A65: every bundled texture reaches the runtime after the first
// frame, and losing them never stops the simulation (lit-colour fallback).
test('the cinematic renderer loads every texture without console errors', async ({ page }) => {
  const problems: string[] = [];
  page.on('console', message => { if (message.type() === 'error' || /Content Security Policy|Refused to/i.test(message.text())) problems.push(message.text()); });
  page.on('pageerror', error => problems.push(error.message));
  await page.goto(`${base}simulator/?body=saturn`);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running physics simulation');
  const canvas = page.locator('canvas').first();
  await expect(canvas).toHaveAttribute('data-textures', /^(\d+)\/\1$/, { timeout: 45000 });
  const [loaded, total] = (await canvas.getAttribute('data-textures'))!.split('/').map(Number);
  expect(loaded).toBe(total);
  expect(total).toBe(14);
  expect(problems).toEqual([]);
});

test('missing textures fall back to lit colours and the simulation keeps running', async ({ page }) => {
  await page.route(/\/textures\/[a-z_]+\.(jpg|png)$/, route => route.fulfill({ status: 404, body: '' }));
  await page.goto(`${base}simulator/`);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running physics simulation');
  const canvas = page.locator('canvas').first();
  await expect(canvas).toHaveAttribute('data-textures', '0/14', { timeout: 45000 });
  const elapsed = page.locator('[data-runtime-elapsed]');
  const before = await elapsed.textContent();
  await expect.poll(async () => elapsed.textContent(), { timeout: 10000 }).not.toBe(before);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running physics simulation');
});

test('the reference grid can be hidden and shown again through C', async ({ page }) => {
  await page.goto(`${base}simulator/`);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running physics simulation');
  await page.getByRole('button', { name: 'View options', exact: true }).click();
  const grid = page.getByRole('button', { name: /^Grid: (On|Off)$/ });
  await expect(grid).toHaveText('Grid: On');
  await expect(grid).toHaveAttribute('aria-pressed', 'true');
  await grid.click();
  await expect(grid).toHaveText('Grid: Off');
  await expect(grid).toHaveAttribute('aria-pressed', 'false');
  await grid.click();
  await expect(grid).toHaveText('Grid: On');
  const labels = page.getByRole('button', { name: /^Labels: (On|Off)$/ });
  await expect(labels).toHaveText('Labels: On');
  await labels.click();
  await expect(labels).toHaveText('Labels: Off');
  await expect(labels).toHaveAttribute('aria-pressed', 'false');
});
