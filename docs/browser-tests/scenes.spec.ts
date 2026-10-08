import { test, expect } from './fixtures';

const base = '/solar-system-simulator/';

// SPEC 2026-10-07 scene split: the main scene keeps the large bodies; every
// small moon runs in its planet's system scene, reachable from a link, the
// scene picker and the catalog.
test('a small moon link opens its planet-system scene and selects it', async ({ page }) => {
  await page.goto(`${base}?body=pan`);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  await expect(page.locator('[data-active-scene]')).toContainText('300 active bodies · Saturn system', { timeout: 30000 });
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Pan;');
  await expect(page.locator('[data-inspector-parent]')).toHaveText('Saturn');
});

test('Pluto, Didymos and Patroclus moons open their small-body system scenes', async ({ page }) => {
  await page.goto(`${base}?body=nix`);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  await expect(page.locator('[data-active-scene]')).toContainText('15 active bodies · Pluto system', { timeout: 30000 });
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Nix;');
  await expect(page.locator('[data-inspector-parent]')).toHaveText('Pluto');
  await page.goto(`${base}?body=dimorphos`);
  await expect(page.locator('[data-active-scene]')).toContainText('11 active bodies · Didymos system', { timeout: 30000 });
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Dimorphos;');
  await expect(page.locator('[data-inspector-parent]')).toHaveText('Didymos');
  await page.goto(`${base}?body=menoetius`);
  await expect(page.locator('[data-active-scene]')).toContainText('11 active bodies · Patroclus system', { timeout: 30000 });
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Menoetius;');
  await expect(page.locator('[data-inspector-parent]')).toHaveText('Patroclus');
  // Pluto and Charon are large bodies: they belong to the main scene.
  await page.goto(`${base}?body=charon`);
  await expect(page.locator('[data-active-scene]')).toContainText('32 active bodies');
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Charon;');
});

test('the scene picker switches between the main scene and planet systems', async ({ page }) => {
  await page.goto(base);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  await expect(page.locator('[data-active-scene]')).toContainText('32 active bodies');
  const scene = page.getByRole('combobox', { name: 'Scene', exact: true });
  await expect(scene).toHaveValue('0');
  await scene.selectOption({ label: 'Uranus (29 moons)' });
  await page.getByRole('button', { name: 'Load scene', exact: true }).click();
  await expect(page.locator('[data-active-scene]')).toContainText('38 active bodies · Uranus system');
  await expect(page.locator('[data-scene-status]')).toContainText('Uranus (29 moons) loaded');
  // The scene opens on its planet, and its catalog fills the body picker.
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Uranus;');
  const bodies = page.getByRole('combobox', { name: 'Body', exact: true, includeHidden: true });
  await expect(bodies.locator('option', { hasText: 'Cordelia' })).toHaveCount(1);
  await scene.selectOption('0');
  await page.getByRole('button', { name: 'Load scene', exact: true }).click();
  await expect(page.locator('[data-active-scene]')).toContainText('32 active bodies · Main scene');
  await expect(bodies.locator('option', { hasText: 'Cordelia' })).toHaveCount(0);
  await expect(bodies.locator('option', { hasText: /^Titan —/ })).toHaveCount(1);
});

test('every catalog row links into the simulator, small moons through their scene', async ({ page }) => {
  await page.goto(`${base}catalog/`);
  await page.getByRole('combobox', { name: 'Family' }).selectOption('Saturn moons');
  await page.getByRole('searchbox', { name: 'Search' }).fill('pan');
  await page.getByRole('link', { name: 'Pan', exact: true }).click();
  await expect(page.locator('[data-active-scene]')).toContainText('Saturn system', { timeout: 30000 });
  await expect(page.locator('[data-runtime-controls]')).toContainText('Selected body: Pan;');
});
