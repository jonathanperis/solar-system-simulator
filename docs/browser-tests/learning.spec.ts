import { test, expect, type Download } from '@playwright/test';
import { resolve } from 'node:path';

const base = '/solar-system-simulator/';
async function downloadText(download: Download): Promise<string> {
  const stream = await download.createReadStream();
  if (!stream) throw new Error('Expected task-owned download stream');
  const chunks: Buffer[] = [];
  for await (const chunk of stream) chunks.push(Buffer.from(chunk));
  return Buffer.concat(chunks).toString('utf8');
}

test('comparison, save/share/import and display units retain reproducible C measurements', async ({ page }) => {
  const errors: string[] = []; page.on('pageerror', error => errors.push(error.message));
  await page.goto(`${base}compare/`);
  await expect(page.locator('[data-lab-status]')).toContainText('Comparison lab ready');
  await page.getByRole('button', { name: 'Start comparison', exact: true }).click();
  await expect(page.getByRole('status')).toHaveText('Comparison complete.');
  await expect(page.locator('[data-matched-time]')).toHaveText('86400 s');
  await expect(page.locator('[data-plot="trajectory"] [data-series]')).toHaveCount(2);
  await expect(page.locator('[data-force-side="0"]')).toContainText('Sun');
  await page.getByText('Save, share, or download', { exact: true }).click();
  const csvBefore = page.waitForEvent('download');
  await page.getByRole('button', { name: 'Export both runs (CSV)' }).click();
  const before = await downloadText(await csvBefore);
  await page.getByLabel('Trajectory display units').selectOption('au');
  await expect(page.locator('[data-trajectory-units]')).toContainText('in AU');
  const csvAfter = page.waitForEvent('download');
  await page.getByRole('button', { name: 'Export both runs (CSV)' }).click();
  expect(await downloadText(await csvAfter)).toBe(before);
  const save = page.waitForEvent('download');
  await page.getByRole('button', { name: 'Save configuration', exact: true }).click();
  const definition = await downloadText(await save);
  expect(definition).toContain('SOLAR_LAB_V1 circular 1 verlet 300 none verlet 150 none 3600 86400');
  await page.getByRole('button', { name: 'Create share link' }).click();
  const link = await page.getByLabel('Shareable experiment URL').inputValue();
  await page.goto(link);
  await expect(page.getByRole('status')).toContainText('Shared configuration loaded');
  await expect(page.getByLabel('Timestep A (seconds)')).toHaveValue('300');
  await expect(page.locator('[data-matched-time]')).toHaveText('—');
  await page.getByText('Save, share, or download', { exact: true }).click();
  await page.getByLabel('Import configuration').setInputFiles(resolve('../examples/collision.solar'));
  await expect(page.getByRole('status')).toContainText('Configuration imported');
  await page.getByRole('button', { name: 'Start comparison', exact: true }).click();
  await expect(page.getByRole('status')).toHaveText('Comparison complete.');
  const present = page.locator('[data-measurements] tr').filter({ has: page.getByRole('cell', { name: 'subject_present', exact: true }) });
  await expect(present.locator('td').nth(2)).toHaveText('0.0000e+0');
  await expect(page.locator('[data-force-side="1"]')).toContainText('Tracked subject merged');
  await page.getByLabel('Import configuration').setInputFiles({ name: 'bad.solar', mimeType: 'text/plain', buffer: Buffer.from('SOLAR_LAB_V99 invalid') });
  await expect(page.getByRole('status')).toContainText('Previous configuration retained');
  await expect(page.getByRole('combobox', { name: 'Preset', exact: true })).toHaveValue('collision');
  await page.getByRole('combobox', { name: 'Preset', exact: true }).selectOption('circular');
  await expect(page.locator('[data-pending-config]')).toBeVisible();
  await expect(page.locator('[data-displayed-definition]')).toContainText('collision');
  expect(errors).toEqual([]);
});

test('Phobos challenge uses analytical measurements rather than visual plausibility', async ({ page }) => {
  await page.goto(`${base}compare/`);
  await expect(page.locator('[data-lab-status]')).toContainText('Comparison lab ready');
  await page.getByLabel('Guided challenges').selectOption('phase');
  await page.getByRole('button', { name: 'Load challenge' }).click();
  await expect(page.locator('[data-challenge-feedback]')).toContainText('100 simulated days');
  await page.getByText('Write a prediction (optional)', { exact: true }).click();
  await page.getByLabel('My prediction').fill('Reducing the timestep should reduce phase error.');
  await page.getByRole('button', { name: 'Start comparison', exact: true }).click();
  await expect(page.getByRole('status')).toHaveText('Comparison complete.', { timeout: 45000 });
  await expect(page.locator('[data-challenge-feedback]')).toContainText('Budget exceeded');
  await page.getByText('Adjust experiment settings', { exact: true }).click();
  await page.getByLabel('Timestep A (seconds)').fill('15');
  await page.getByRole('button', { name: 'Start comparison', exact: true }).click();
  await expect(page.getByRole('status')).toHaveText('Comparison complete.', { timeout: 45000 });
  await expect(page.locator('[data-challenge-feedback]')).toContainText('Budget met');
});

test('mobile atlas controls remain distinct and keyboard bearing stays continuous', async ({ page }) => {
  for (const width of [320, 390]) {
    await page.setViewportSize({ width, height: 844 });
    await page.goto(base);
    const controls = page.locator('[data-atlas-plate="heliocentric"] [data-atlas-body]');
    await expect(controls).toHaveCount(10);
    const boxes = await Promise.all((await controls.all()).map(control => control.boundingBox()));
    for (let i = 0; i < boxes.length; i++) for (let j = i + 1; j < boxes.length; j++) {
      const a = boxes[i]!, b = boxes[j]!;
      expect(Math.min(a.x+a.width, b.x+b.width) > Math.max(a.x,b.x) && Math.min(a.y+a.height,b.y+b.height) > Math.max(a.y,b.y), `marker overlap ${width}px ${i}/${j}`).toBe(false);
    }
    for (const name of ['Vesta', 'Uranus']) {
      await page.getByRole('button', { name, exact: true }).click();
      await expect(page.getByRole('heading', { name, exact: true })).toBeVisible();
      if (name === 'Uranus') {
        await page.getByText('Model and sources', { exact: true }).click();
        await page.getByRole('link', { name: 'Open catalog', exact: true }).scrollIntoViewIfNeeded();
        const sheet = await page.locator('[data-atlas-note]').boundingBox();
        const close = await page.getByRole('button', { name: 'Close body detail' }).boundingBox();
        expect(close!.y).toBeGreaterThanOrEqual(sheet!.y);
        expect(close!.y + close!.height).toBeLessThanOrEqual(sheet!.y + sheet!.height);
      }
      await page.getByRole('button', { name: 'Close body detail' }).focus();
      await page.keyboard.press('Escape');
      await expect(page.getByRole('button', { name, exact: true })).toBeFocused();
    }
    await page.getByRole('slider', { name: 'Observation bearing' }).focus();
    await page.keyboard.press('Home'); await page.keyboard.press('ArrowRight');
    await expect(page.getByRole('slider', { name: 'Observation bearing' })).toHaveValue('1');
  }
});

test('3D controls export the same SI snapshot across render scales', async ({ page }) => {
  await page.goto(`${base}simulator/`);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running physics simulation');
  await page.getByRole('button', { name: 'Pause', exact: true }).click();
  await page.getByRole('button', { name: 'Learn', exact: true }).click();
  await page.getByRole('combobox', { name: 'Lesson preset', exact: true }).selectOption('1');
  await page.getByRole('button', { name: 'Load lesson', exact: true }).click();
  await page.getByRole('button', { name: 'Close learning activities' }).click();
  await page.getByRole('button', { name: 'Advanced', exact: true }).click();
  await page.getByRole('button', { name: 'Step +15 s', exact: true }).click();
  await expect(page.locator('[data-runtime-elapsed]')).toContainText('15 s');
  await expect(page.locator('[data-runtime-forces]')).toContainText('Sun');
  const first = page.waitForEvent('download'); await page.getByRole('button', { name: 'Download measurements (CSV)' }).click();
  const before = await downloadText(await first);
  await page.getByRole('button', { name: 'Close advanced tools' }).click();
  await page.getByRole('button', { name: 'View options', exact: true }).click();
  await page.getByRole('button', { name: 'View: Illustrative', exact: true }).click();
  await page.getByRole('button', { name: 'Close view options' }).click();
  await page.getByRole('button', { name: 'Advanced', exact: true }).click();
  const second = page.waitForEvent('download'); await page.getByRole('button', { name: 'Download measurements (CSV)' }).click();
  expect(await downloadText(await second)).toBe(before);
  await page.getByRole('button', { name: 'Close advanced tools' }).click();
  await page.getByRole('button', { name: 'Learn', exact: true }).click();
  await page.getByRole('combobox', { name: 'Lesson preset', exact: true }).selectOption({ label: 'Head-on collisions' });
  await page.getByRole('button', { name: 'Load lesson', exact: true }).click();
  await page.getByRole('button', { name: 'Contact: Elastic bounce (restart)', exact: true }).click();
  await page.getByRole('button', { name: 'Close learning activities' }).click();
  await page.getByRole('combobox', { name: 'Simulation speed', exact: true }).selectOption('4');
  await page.getByRole('button', { name: 'Resume', exact: true }).click();
  const bodies = page.getByRole('combobox', { name: 'Selected body', exact: true, includeHidden: true }).locator('option');
  await expect(bodies).toHaveCount(1);
  await expect(page.locator('[data-runtime-forces]')).toContainText('No other known-mass gravitational sources.');
  await page.getByRole('button', { name: 'Pause', exact: true }).click();
  await page.getByRole('button', { name: 'Restart', exact: true }).click();
  await expect(bodies).toHaveCount(2);
  await page.getByRole('button', { name: 'Learn', exact: true }).click();
  await page.getByRole('combobox', { name: 'Lesson preset', exact: true }).selectOption({ label: 'Moving-Sun barycentric core' });
  await page.getByRole('button', { name: 'Load lesson', exact: true }).click();
  await expect(page.locator('[data-active-scene]')).toContainText('128 active bodies');
  await expect(page.getByLabel('Time per calculation (seconds)')).toHaveValue('15');
  await page.getByRole('button', { name: 'Close learning activities' }).click();
  await page.getByRole('button', { name: 'Advanced', exact: true }).click();
  await page.getByRole('button', { name: 'Step +15 s', exact: true }).click();
  await expect(page.locator('[data-runtime-elapsed]')).toContainText('15 s');
});

test('real missing/invalid local assets fail visibly and disable runtime controls', async ({ page }) => {
  for (const [path, status, panel] of [
    ['/__test__/missing-runtime/', '[data-runtime-status]', '[data-runtime-panel]'],
    ['/__test__/bad-runtime/', '[data-runtime-status]', '[data-runtime-panel]'],
    ['/__test__/bad-lab/', '[data-lab-status]', 'fieldset']
  ]) {
    await page.goto(path);
    await expect(page.locator(status)).toContainText(/Runtime error|Comparison runtime unavailable/);
    for (const fieldset of await page.locator(panel).all()) await expect(fieldset).toHaveAttribute('disabled', '');
    await expect(page.locator(panel).first().locator('button').first()).toBeDisabled();
  }
});
