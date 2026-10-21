import { test, expect } from './fixtures';

const base = '/';

// SPEC A64-A65: every bundled texture reaches the runtime after the first
// frame, and losing them never stops the simulation (lit-colour fallback).
test('the cinematic renderer loads every texture without console errors', async ({ page }) => {
  const problems: string[] = [];
  page.on('console', message => { if (message.type() === 'error' || /Content Security Policy|Refused to/i.test(message.text())) problems.push(message.text()); });
  page.on('pageerror', error => problems.push(error.message));
  await page.goto(`${base}?body=saturn`);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  const canvas = page.locator('canvas').first();
  await expect(canvas).toHaveAttribute('data-textures', /^(\d+)\/\1$/, { timeout: 45000 });
  const [loaded, total] = (await canvas.getAttribute('data-textures'))!.split('/').map(Number);
  expect(loaded).toBe(total);
  expect(total).toBe(28); // 14 Solar System Scope maps + 14 USGS moon maps (A109)
  expect(problems).toEqual([]);
});

test('missing textures fall back to lit colours and the simulation keeps running', async ({ page }) => {
  await page.route(/\/textures\/[a-z_]+\.(jpg|png)$/, route => route.fulfill({ status: 404, body: '' }));
  await page.goto(base);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  const canvas = page.locator('canvas').first();
  await expect(canvas).toHaveAttribute('data-textures', '0/28', { timeout: 45000 });
  const elapsed = page.locator('[data-runtime-elapsed]');
  const before = await elapsed.textContent();
  await expect.poll(async () => elapsed.textContent(), { timeout: 10000 }).not.toBe(before);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
});

test('the reference grid can be hidden and shown again through C', async ({ page }) => {
  await page.goto(base);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  await page.getByRole('button', { name: 'View', exact: true }).click();
  const grid = page.getByRole('switch', { name: 'Grid', exact: true });
  await expect(grid).toBeChecked();
  await expect(grid).toBeChecked();
  await grid.click();
  await expect(grid).not.toBeChecked();
  await expect(grid).not.toBeChecked();
  await grid.click();
  await expect(grid).toBeChecked();
  const labels = page.getByRole('switch', { name: 'Labels', exact: true });
  await expect(labels).toBeChecked();
  await labels.click();
  await expect(labels).not.toBeChecked();
  await expect(labels).not.toBeChecked();
});

test('orbit guides start on with parent-relative trails and toggle through C', async ({ page }) => {
  await page.goto(base);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  await page.getByRole('button', { name: 'View', exact: true }).click();
  await expect(page.getByRole('button', { name: 'Parent', exact: true })).toHaveAttribute('aria-pressed', 'true');
  const orbits = page.getByRole('switch', { name: 'Orbits', exact: true });
  await expect(orbits).toBeChecked();
  await expect(orbits).toBeChecked();
  await orbits.click();
  await expect(orbits).not.toBeChecked();
  await expect(orbits).not.toBeChecked();
  await page.keyboard.press('Escape');
  await page.locator('canvas').focus();
  // raylib polls keys once a frame: hold the key across a few frames.
  await page.keyboard.down('o');
  await page.waitForTimeout(150);
  await page.keyboard.up('o');
  await page.getByRole('button', { name: 'View', exact: true }).click();
  await expect(orbits).toBeChecked();
});

// SPEC A112: reduced motion stops auto-rotation (and fly-to animation), and
// dragging the canvas turns the view.
test('reduced motion starts still, and a drag turns the paused view', async ({ page }) => {
  await page.emulateMedia({ reducedMotion: 'reduce' });
  await page.goto(base);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  await page.getByRole('button', { name: 'View', exact: true }).click();
  await expect(page.getByRole('switch', { name: 'Rotate', exact: true })).not.toBeChecked();
  await page.keyboard.press('Escape');
  await page.getByRole('button', { name: 'Pause', exact: true }).click();
  await expect(page.locator('[data-runtime-status]')).toHaveText('Paused');
  const canvas = page.locator('canvas').first();
  await expect(canvas).toHaveAttribute('data-textures', /^(\d+)\/\1$/, { timeout: 45000 }); // no late texture changes the frame
  await page.waitForTimeout(300);
  const still = await canvas.screenshot();
  await page.waitForTimeout(300);
  expect((await canvas.screenshot()).equals(still)).toBe(true); // nothing moves on its own
  const box = (await canvas.boundingBox())!;
  await page.mouse.move(box.x + box.width / 2, box.y + box.height / 2);
  await page.mouse.down();
  for (let step = 1; step <= 10; step++) {
    await page.mouse.move(box.x + box.width / 2 + step * 12, box.y + box.height / 2 + step * 4);
    await page.waitForTimeout(20);
  }
  await page.mouse.up();
  await page.waitForTimeout(200);
  expect((await canvas.screenshot()).equals(still)).toBe(false);
});

// SPEC A112, A118: pointing at a body rings it and shows a pointer cursor;
// clicking the selection zooms in to it. Jupiter, framed by its link, sits at
// the centre of the view, so the test needs no search (CI renders slowly).
test('hovering a body shows the pointer and clicking the selection zooms in', async ({ page }) => {
  await page.emulateMedia({ reducedMotion: 'reduce' });
  await page.goto(`${base}?body=jupiter`);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  await page.getByRole('button', { name: 'Pause', exact: true }).click();
  const canvas = page.locator('canvas').first();
  const box = (await canvas.boundingBox())!;
  await page.mouse.move(box.x + box.width / 2, box.y + box.height / 2);
  await expect.poll(() => canvas.evaluate(element => 'hover' in (element as HTMLElement).dataset), { timeout: 20000 }).toBe(true);
  await expect(canvas).toHaveCSS('cursor', 'pointer');
  const zoom = page.locator('[data-runtime-camera]');
  const before = await zoom.textContent();
  await page.mouse.down();
  await page.waitForTimeout(150); // a real click spans frames; raylib polls once per frame
  await page.mouse.up();
  await expect(page.locator('[data-inspector-name]')).toHaveText('Jupiter');
  await expect.poll(() => zoom.textContent(), { timeout: 20000 }).not.toBe(before); // framed closer
});

// SPEC A119: from a close-up, off-screen planets are marked at the view's
// edge; clicking one flies there, and Escape steps back out to the Sun.
test('edge markers navigate between planets and Escape steps back out', async ({ page }) => {
  test.setTimeout(150000);
  await page.emulateMedia({ reducedMotion: 'reduce' });
  await page.goto(`${base}?body=venus`);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  await page.getByRole('button', { name: 'Pause', exact: true }).click();
  await page.getByRole('button', { name: 'Frame body', exact: true }).click();
  const canvas = page.locator('canvas').first();
  const box = (await canvas.boundingBox())!;
  const hovering = () => canvas.evaluate(element => 'hover' in (element as HTMLElement).dataset);
  let hit: [number, number] | null = null;
  // Walk the left and right edges, where markers for off-screen planets sit.
  // Each step waits two real frames: CI renders slowly, so fixed delays miss.
  const frames = () => page.evaluate(() => new Promise(done => requestAnimationFrame(() => requestAnimationFrame(() => done(null)))));
  for (const x of [box.x + 38, box.x + box.width - 38]) {
    for (let y = box.y + 20; y < box.y + box.height - 20 && !hit; y += 10) {
      await page.mouse.move(x, y);
      await frames();
      if (await hovering()) hit = [x, y];
    }
    if (hit) break;
  }
  expect(hit).not.toBeNull();
  await page.mouse.down();
  await page.waitForTimeout(80);
  await page.mouse.up();
  const name = page.locator('[data-inspector-name]');
  await expect(name).not.toHaveText('Venus');
  await canvas.focus();
  for (let step = 0; step < 2; step++) {
    await page.keyboard.down('Escape');
    await page.waitForTimeout(80);
    await page.keyboard.up('Escape');
    await page.waitForTimeout(150);
  }
  await expect(name).toHaveText('Sun');
});

// SPEC A120: arrow keys hop between bodies, the address follows the
// selection and browser Back retraces it; "/" opens search from the view;
// Tour and Follow are C toggles.
test('arrows, address, Back, search, tour and follow drive navigation', async ({ page }) => {
  await page.emulateMedia({ reducedMotion: 'reduce' });
  await page.goto(`${base}?body=venus`);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  const name = page.locator('[data-inspector-name]');
  await expect(name).toHaveText('Venus');
  const canvas = page.locator('canvas').first();
  const press = async (key: string) => {
    await canvas.focus();
    await page.keyboard.down(key);
    await page.waitForTimeout(80);
    await page.keyboard.up(key);
  };
  await press('ArrowRight');
  await expect(name).toHaveText('Earth');
  await expect(page).toHaveURL(/\?body=earth$/);
  await press('ArrowDown');
  await expect(name).toHaveText('Moon');
  await expect(page).toHaveURL(/\?body=moon$/);
  // Backspace on the view goes back one body (review fix: it used to be
  // swallowed by the GLFW key filter), and the browser's Back goes further.
  await press('Backspace');
  await expect(name).toHaveText('Earth');
  await page.goBack();
  await expect(name).toHaveText('Venus');
  // The deep link's own entry survived (review fix): Venus is not replaced
  // by the Sun, and Backspace does not leave the site from the first entry.
  await expect(page).toHaveURL(/\?body=venus$/);
  await press('Backspace');
  await expect(page).toHaveURL(/\?body=venus$/);
  await canvas.focus();
  await page.keyboard.press('/');
  await expect(page.locator('#find-panel')).toBeVisible();
  await expect(page.getByRole('searchbox', { name: 'Name' })).toBeFocused();
  await page.keyboard.press('Escape');
  const tour = page.getByRole('button', { name: /^(Tour|Stop tour)$/ });
  await tour.click();
  await expect(tour).toHaveText('Stop tour');
  await expect(name).toHaveText('Sun');
  await tour.click();
  await expect(tour).toHaveText('Tour');
  await page.getByRole('button', { name: 'View', exact: true }).click();
  const follow = page.getByRole('switch', { name: 'Follow', exact: true });
  await follow.click();
  await expect(follow).toBeChecked();
  await expect(follow).toBeChecked();
});

// SPEC A121: the small-body sample loads after the textures and the cloud
// toggles through C.
test('the small-body cloud loads and toggles', async ({ page }) => {
  await page.goto(base);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  await expect(page.locator('canvas').first()).toHaveAttribute('data-belt', 'loaded', { timeout: 60000 });
  await page.getByRole('button', { name: 'View', exact: true }).click();
  const belt = page.getByRole('switch', { name: 'Asteroids', exact: true });
  await expect(belt).toBeChecked();
  await belt.click();
  await expect(belt).not.toBeChecked();
  await expect(belt).not.toBeChecked();
});

// SPEC A124: constellation lines start off and toggle through C.
test('constellation lines toggle through C', async ({ page }) => {
  await page.goto(base);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  await page.getByRole('button', { name: 'View', exact: true }).click();
  const lines = page.getByRole('switch', { name: 'Constellations', exact: true });
  await expect(lines).not.toBeChecked();
  await lines.click();
  await expect(lines).toBeChecked();
  await expect(lines).toBeChecked();
});

// SPEC A128: the View panel is non-modal: the canvas stays usable behind it,
// Escape closes it and focus returns to its opener; the scale is a segmented
// control and on/off controls are switches.
test('the View panel is non-modal, with switches and segmented controls', async ({ page }) => {
  await page.goto(base);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  const opener = page.getByRole('button', { name: 'View', exact: true });
  await opener.click();
  const panel = page.locator('#view-panel');
  await expect(panel).toBeVisible();
  await expect(opener).toHaveAttribute('aria-expanded', 'true');
  // Not modal: the dialog is open without a backdrop, and the dock stays clickable.
  expect(await panel.evaluate(element => element.matches(':modal'))).toBe(false);
  await page.getByRole('button', { name: /^(Pause|Resume)$/ }).click();
  await expect(page.locator('[data-runtime-status]')).toHaveText('Paused');
  const real = page.getByRole('button', { name: 'Real', exact: true });
  await real.click();
  await expect(real).toHaveAttribute('aria-pressed', 'true');
  await expect(page.getByRole('button', { name: 'Illustrative', exact: true })).toHaveAttribute('aria-pressed', 'false');
  const sun = page.getByRole('button', { name: 'Sun', exact: true });
  await sun.click();
  await expect(sun).toHaveAttribute('aria-pressed', 'true');
  await panel.getByRole('switch', { name: 'Grid', exact: true }).focus();
  await page.keyboard.press('Escape');
  await expect(panel).toBeHidden();
  await expect(opener).toBeFocused();
  await expect(opener).toHaveAttribute('aria-expanded', 'false');
  // The Close button also hands focus back (non-modal dialogs do not).
  await opener.click();
  await page.getByRole('button', { name: 'Close view' }).click();
  await expect(panel).toBeHidden();
  await expect(opener).toBeFocused();
});

// The View panel is a bottom sheet on short landscape screens, where the
// instrument stacks its panels.
test('the View panel is a bottom sheet on a short landscape screen', async ({ page }) => {
  await page.setViewportSize({ width: 900, height: 420 });
  await page.goto(base);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  await page.getByRole('button', { name: 'View', exact: true }).click();
  const box = (await page.locator('#view-panel').boundingBox())!;
  expect(box.width).toBeGreaterThan(880);
  expect(Math.abs(box.y + box.height - 420)).toBeLessThan(2);
});
