import type { Download } from '@playwright/test';
import { test, expect } from './fixtures';
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
  await page.goto(`${base}learn/compare/`);
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
  await page.goto(`${base}learn/compare/`);
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

test('3D controls export the same SI snapshot across render scales', async ({ page }) => {
  await page.goto(`${base}?lesson=circular`);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  await expect(page.locator('[data-active-scene]')).toContainText('Circular orbit');
  await page.getByRole('button', { name: 'Pause', exact: true }).click();
  await page.getByRole('button', { name: 'Restart', exact: true }).click();
  await expect(page.locator('[data-runtime-elapsed]')).toContainText('(0 s)');
  const openSheet = async (name: string) => page.getByRole('button', { name, exact: true }).click();
  await openSheet('Data');
  await page.getByRole('button', { name: 'Step +15 s', exact: true }).click();
  await expect(page.locator('[data-runtime-elapsed]')).toContainText('(15 s)');
  await page.getByText('Gravity sources at').click();
  await expect(page.locator('[data-runtime-forces]')).toContainText('Sun');
  const first = page.waitForEvent('download'); await page.getByRole('button', { name: 'Download CSV' }).click();
  const before = await downloadText(await first);
  await page.getByRole('button', { name: 'Close data' }).click();
  await openSheet('View');
  await page.getByRole('button', { name: 'View: Illustrative', exact: true }).click();
  await page.getByRole('button', { name: 'Close view' }).click();
  await openSheet('Data');
  const second = page.waitForEvent('download'); await page.getByRole('button', { name: 'Download CSV' }).click();
  expect(await downloadText(await second)).toBe(before);
  await page.getByRole('button', { name: 'Close data' }).click();

  // The lesson strip validates its own fields and keeps the previous lesson.
  const lesson = page.getByRole('combobox', { name: 'Lesson', exact: true });
  const timestep = page.getByRole('spinbutton', { name: 'Step (s)' });
  await lesson.selectOption({ label: 'Head-on collisions' });
  await timestep.fill('0.5');
  await page.getByRole('button', { name: 'Load', exact: true }).click();
  await expect(timestep).toBeFocused();
  await expect(page.locator('[data-lesson-status]')).toContainText('0.01 to 0.25 seconds');
  await expect(page.locator('[data-active-scene]')).toContainText('Circular orbit');
  await timestep.fill('0.1');
  await page.getByRole('button', { name: 'Load', exact: true }).click();
  await page.getByRole('button', { name: 'Contact: Elastic bounce (restart)', exact: true }).click();
  await page.getByRole('combobox', { name: 'Speed', exact: true }).selectOption('4');
  await page.getByRole('button', { name: 'Resume', exact: true }).click();
  const bodies = page.getByRole('combobox', { name: 'Body', exact: true, includeHidden: true }).locator('option');
  await expect(bodies).toHaveCount(1);
  await expect(page.locator('[data-runtime-forces]')).toContainText('No other known-mass gravitational sources.');
  await page.getByRole('button', { name: 'Pause', exact: true }).click();
  await page.getByRole('button', { name: 'Restart', exact: true }).click();
  await expect(bodies).toHaveCount(2);
  await lesson.selectOption({ label: 'Moving-Sun barycentric core' });
  const factor = page.getByRole('spinbutton', { name: 'Speed ×' });
  await factor.fill('1.1');
  await page.getByRole('button', { name: 'Load', exact: true }).click();
  await expect(factor).toBeFocused();
  await expect(page.locator('[data-lesson-status]')).toContainText('multiplier of 1');
  await expect(page.locator('[data-active-scene]')).toContainText('Head-on collisions');
  await factor.fill('1');
  await page.getByRole('button', { name: 'Load', exact: true }).click();
  await expect(page.locator('[data-active-scene]')).toContainText('32 active bodies');
  await expect(timestep).toHaveValue('15');
  await openSheet('Data');
  await page.getByRole('button', { name: 'Step +15 s', exact: true }).click();
  // The barycentric core starts from the dated sky, so its clock is a date.
  await expect(page.locator('[data-runtime-elapsed]')).toHaveText('2026-06-09 00:00 TDB');
  await expect(page.locator('[data-forces-time]')).toHaveText('15 simulated seconds');
  // Leaving the lesson for the main scene hides the strip again.
  await page.getByRole('button', { name: 'Main scene', exact: true }).click();
  await expect(page.locator('[data-lesson-strip]')).toBeHidden();
});

test('real missing/invalid local assets fail visibly and disable runtime controls', async ({ page }) => {
  for (const [path, status, panel] of [
    ['/__test__/missing-runtime/', '[data-runtime-status]', '[data-runtime-panel]'],
    ['/__test__/bad-runtime/', '[data-runtime-status]', '[data-runtime-panel]'],
    ['/__test__/bad-lab/', '[data-lab-status]', 'fieldset']
  ]) {
    await page.goto(path);
    await expect(page.locator(status)).toContainText(/Runtime error|Comparison runtime unavailable/);
    for (const fieldset of await page.locator(panel).all()) {
      await expect(fieldset).toHaveAttribute('disabled', '');
      await expect(fieldset.locator('button').first()).toBeDisabled();
    }
  }
});
