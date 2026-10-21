#!/usr/bin/env node
// Renders a preview image per guided lesson for the Learn page (SPEC A129):
// docs/public/lessons/<name>.jpg, 480x300, from the running simulator.
//
// Usage: build the site, serve it, then pass its base URL, e.g.
//   python3 tools/serve_site.py docs/dist --port 4400 &
//   node tools/lesson_previews.mjs http://127.0.0.1:4400/
// then rebuild (npm run build --prefix docs). Uses the pinned project
// Playwright with installed Chrome (WebGL needs it).
//
// Each lesson runs at its fastest speed with camera rotation off until a
// lesson-specific simulated time has passed, then pauses. Frames are
// illustrative: a rerun can differ by a frame or two of motion.
import { createRequire } from 'node:module';
import { mkdirSync, writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';

const root = fileURLToPath(new URL('..', import.meta.url));
const out = `${root}docs/public/lessons/`;
const { chromium } = createRequire(`${root}docs/package.json`)('playwright');
const base = process.argv[2];
if (!base?.endsWith('/')) throw new Error('pass the served site base URL, ending in /');

// Lesson, how much simulated time to show (seconds: enough for its trails to
// draw the shape the lesson is about) and framing: the whole scene around
// the Sun, the selected body's system, or the lesson's own opening frame.
const day = 86400;
const lessons = [
  ['circular', 200 * day, 'scene'], ['eccentric', 300 * day, 'scene'], ['escape', 250 * day, 'scene'],
  ['inclined', 250 * day, 'scene'], ['earth-moon', 25 * day, 'system'], ['pluto-charon', 6 * day, 'lesson'],
  ['dart', 1 * day, 'lesson'], ['phobos', 0.6 * day, 'system'], ['barycentric-core', 400 * day, 'scene'],
  ['resonance', 900 * day, 'scene'], ['encounter', 3 * day, 'system'], ['collision', 400, 'lesson'],
];

mkdirSync(out, { recursive: true });
const browser = await chromium.launch({ channel: 'chrome' });
for (const [name, target, framing] of lessons) {
  const page = await browser.newPage({ viewport: { width: 960, height: 700 }, bypassCSP: true });
  await page.goto(`${base}?lesson=${name}`);
  await page.locator('[data-runtime-status]').filter({ hasText: /^Running$/ }).waitFor({ timeout: 90000 });
  await page.waitForFunction(() => /^(\d+)\/\1$/.test(document.querySelector('canvas')?.dataset.textures ?? ''), null, { timeout: 90000 });
  const header = await page.locator('.site-header').evaluate(element => element.getBoundingClientRect().height);
  await page.setViewportSize({ width: 960, height: 600 + Math.round(header) });
  await page.addStyleTag({ content: '.hud,.dock,.inspector,.lesson-strip{display:none!important}' });
  await page.evaluate(() => {
    const rotate = document.querySelector('[data-runtime-rotate]');
    if (rotate?.getAttribute('aria-checked') === 'true') rotate.click();
    const speed = document.querySelector('[data-runtime-speed]');
    speed.value = speed.max;
    speed.dispatchEvent(new Event('input', { bubbles: true }));
  });
  await page.waitForFunction(t => Number(document.querySelector('[data-runtime-elapsed]')?.dataset.seconds) >= t,
    target, { timeout: 120000, polling: 100 });
  await page.evaluate(framing => {
    document.querySelector('[data-command="pause"]').click();
    // Thumbnails are small: names would be unreadable smudges.
    const labels = document.querySelector('[data-command="labels"]');
    if (labels?.getAttribute('aria-checked') === 'true') labels.click();
    if (framing === 'scene') document.querySelector('[data-whole-system]').click();
    if (framing === 'system') document.querySelector('[data-command="frame"]').click();
  }, framing);
  await page.waitForTimeout(2200); // the framing flight
  const frame = await page.locator('canvas').screenshot({ type: 'jpeg', quality: 78 });
  // Downscale in the browser so the file stays small and sharp.
  const small = await page.evaluate(async bytes => {
    const image = await createImageBitmap(new Blob([new Uint8Array(bytes)], { type: 'image/jpeg' }));
    const canvas = new OffscreenCanvas(480, 300);
    canvas.getContext('2d').drawImage(image, 0, 0, 480, 300);
    const blob = await canvas.convertToBlob({ type: 'image/jpeg', quality: 0.8 });
    return [...new Uint8Array(await blob.arrayBuffer())];
  }, [...frame]);
  writeFileSync(`${out}${name}.jpg`, Buffer.from(small));
  console.log(`${name}: ${small.length} bytes`);
  await page.close();
}
await browser.close();
