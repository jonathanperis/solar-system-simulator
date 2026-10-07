import { test, expect } from './fixtures';

const base = '/solar-system-simulator/';

// SPEC 2026-10-07 scene split: the main scene keeps the large bodies; every
// small moon runs in its planet's system scene, reachable from a link, the
// scene picker and the atlas.
test('a small moon link opens its planet-system scene and selects it', async ({ page }) => {
  await page.goto(`${base}simulator/?body=pan`);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running physics simulation');
  await expect(page.locator('[data-active-scene]')).toContainText('300 active bodies · Saturn system', { timeout: 30000 });
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Pan;');
  await expect(page.locator('[data-inspector-parent]')).toHaveText('Saturn');
});

test('Pluto and Didymos moons open their small-body system scenes', async ({ page }) => {
  await page.goto(`${base}simulator/?body=nix`);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running physics simulation');
  await expect(page.locator('[data-active-scene]')).toContainText('15 active bodies · Pluto system', { timeout: 30000 });
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Nix;');
  await expect(page.locator('[data-inspector-parent]')).toHaveText('Pluto');
  await page.goto(`${base}simulator/?body=dimorphos`);
  await expect(page.locator('[data-active-scene]')).toContainText('11 active bodies · Didymos system', { timeout: 30000 });
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Dimorphos;');
  await expect(page.locator('[data-inspector-parent]')).toHaveText('Didymos');
  // Pluto and Charon are large bodies: they belong to the main scene.
  await page.goto(`${base}simulator/?body=charon`);
  await expect(page.locator('[data-active-scene]')).toContainText('32 active bodies');
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Charon;');
});

test('the scene picker switches between the main scene and planet systems', async ({ page }) => {
  await page.goto(`${base}simulator/`);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running physics simulation');
  await expect(page.locator('[data-active-scene]')).toContainText('32 active bodies');
  await page.getByRole('button', { name: 'Find an object', exact: true }).click();
  const scene = page.getByRole('combobox', { name: 'Scene', exact: true });
  await expect(scene).toHaveValue('0');
  await scene.selectOption({ label: 'Uranus system — all 29 moons' });
  await page.getByRole('button', { name: 'Load scene', exact: true }).click();
  await expect(page.locator('[data-active-scene]')).toContainText('38 active bodies · Uranus system');
  await expect(page.locator('[data-scene-status]')).toContainText('Uranus system — all 29 moons loaded');
  // The scene opens on its planet, and its catalog fills the body picker.
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Uranus;');
  await expect(page.getByRole('combobox', { name: 'Selected body' }).locator('option', { hasText: 'Cordelia' })).toHaveCount(1);
  await scene.selectOption('0');
  await page.getByRole('button', { name: 'Load scene', exact: true }).click();
  await expect(page.locator('[data-active-scene]')).toContainText('32 active bodies · Main scene');
  await expect(page.getByRole('combobox', { name: 'Selected body' }).locator('option', { hasText: 'Cordelia' })).toHaveCount(0);
  await expect(page.getByRole('combobox', { name: 'Selected body' }).locator('option', { hasText: /^Titan —/ })).toHaveCount(1);
});

test('the atlas pages through every Saturn moon group', async ({ page }) => {
  await page.goto(base);
  await page.getByRole('button', { name: /Saturn system/ }).click();
  const group = page.getByRole('combobox', { name: 'Moon group' });
  await expect(group).toHaveValue('Major moons');
  await expect(page.locator('[data-atlas-plate="saturn"] [data-atlas-body]:not([hidden])')).toHaveCount(6);
  await group.selectOption('Irregular moons');
  await expect(page.locator('[data-atlas-page]')).toHaveText('Page 1 of 45');
  await page.getByRole('button', { name: 'Next page', exact: true }).click();
  await expect(page.locator('[data-atlas-page]')).toHaveText('Page 2 of 45');
  await page.locator('[data-atlas-plate="saturn"] [data-atlas-body]:not([hidden])').first().click();
  await expect(page.locator('[data-atlas-note] a', { hasText: /in the simulator/ })).toHaveAttribute('href', /simulator\/\?body=/);
});
