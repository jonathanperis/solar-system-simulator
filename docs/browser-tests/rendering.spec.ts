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

test('orbit guides start on with parent-relative trails and toggle through C', async ({ page }) => {
  await page.goto(base);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  await page.getByRole('button', { name: 'View', exact: true }).click();
  await expect(page.getByRole('button', { name: /^Trails: / })).toHaveText('Trails: Parent-relative');
  const orbits = page.getByRole('button', { name: /^Orbits: (On|Off)$/ });
  await expect(orbits).toHaveText('Orbits: On');
  await expect(orbits).toHaveAttribute('aria-pressed', 'true');
  await orbits.click();
  await expect(orbits).toHaveText('Orbits: Off');
  await expect(orbits).toHaveAttribute('aria-pressed', 'false');
  await page.keyboard.press('Escape');
  await page.locator('canvas').focus();
  // raylib polls keys once a frame: hold the key across a few frames.
  await page.keyboard.down('o');
  await page.waitForTimeout(150);
  await page.keyboard.up('o');
  await page.getByRole('button', { name: 'View', exact: true }).click();
  await expect(orbits).toHaveText('Orbits: On');
});

// SPEC A112: reduced motion stops auto-rotation (and fly-to animation), and
// dragging the canvas turns the view.
test('reduced motion starts still, and a drag turns the paused view', async ({ page }) => {
  await page.emulateMedia({ reducedMotion: 'reduce' });
  await page.goto(base);
  await expect(page.locator('[data-runtime-status]')).toHaveText('Running');
  await page.getByRole('button', { name: 'View', exact: true }).click();
  await expect(page.locator('[data-runtime-rotate]')).not.toBeChecked();
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
  await page.goBack();
  await expect(name).toHaveText('Earth');
  await page.goBack();
  await expect(name).toHaveText('Venus');
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
  const follow = page.getByRole('button', { name: /^Follow: (On|Off)$/ });
  await follow.click();
  await expect(follow).toHaveText('Follow: On');
  await expect(follow).toHaveAttribute('aria-pressed', 'true');
});
