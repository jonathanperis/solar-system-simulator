#!/usr/bin/env node
// Renders the site's icons and link-preview card into docs/public/:
//   favicon.ico (16/32/48), favicon-32x32.png, apple-touch-icon.png from the
//   hand-written docs/public/favicon.svg, and social-preview.png (1200×630,
//   the og:image) from a live frame of the simulator beside a notebook title.
//
// Usage: build the site, serve it, then pass its base URL, e.g.
//   python3 tools/serve_site.py docs/dist --port 4400 &
//   node tools/site_images.mjs http://127.0.0.1:4400/solar-system-simulator/
// Uses the pinned project Playwright with installed Chrome (WebGL needs it).
import { createRequire } from 'node:module';
import { readFileSync, writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';

const root = fileURLToPath(new URL('..', import.meta.url));
const publicDir = `${root}docs/public/`;
const { chromium } = createRequire(`${root}docs/package.json`)('playwright');
const base = process.argv[2];
if (!base?.endsWith('/')) throw new Error('pass the served site base URL, ending in /');

const browser = await chromium.launch({ channel: 'chrome' });

async function renderSvg(svg, size, transparent) {
  const page = await browser.newPage({ viewport: { width: size, height: size } });
  await page.setContent(`<body style="margin:0;background:transparent"><img width="${size}" height="${size}" src="data:image/svg+xml;base64,${Buffer.from(svg).toString('base64')}"></body>`);
  const png = await page.screenshot({ omitBackground: transparent });
  await page.close();
  return png;
}

// An ICO file whose entries are PNG images (supported by every current browser).
function ico(images) {
  const header = Buffer.alloc(6 + 16 * images.length);
  header.writeUInt16LE(1, 2);
  header.writeUInt16LE(images.length, 4);
  let offset = header.length;
  images.forEach(({ size, png }, i) => {
    const entry = 6 + 16 * i;
    header.writeUInt8(size % 256, entry);
    header.writeUInt8(size % 256, entry + 1);
    header.writeUInt16LE(1, entry + 4);
    header.writeUInt16LE(32, entry + 6);
    header.writeUInt32LE(png.length, entry + 8);
    header.writeUInt32LE(offset, entry + 12);
    offset += png.length;
  });
  return Buffer.concat([header, ...images.map(image => image.png)]);
}

const svg = readFileSync(`${publicDir}favicon.svg`, 'utf8');
const sizes = [16, 32, 48];
const icons = await Promise.all(sizes.map(async size => ({ size, png: await renderSvg(svg, size, true) })));
writeFileSync(`${publicDir}favicon.ico`, ico(icons));
writeFileSync(`${publicDir}favicon-32x32.png`, icons[1].png);
// iOS draws its own rounded corners and fills transparency with black.
writeFileSync(`${publicDir}apple-touch-icon.png`, await renderSvg(svg.replace('rx="14"', 'rx="0"'), 180, false));

// A real frame of the running simulator: Jupiter and the Galilean moons, four
// simulated days after the 2026-06-09 sky. Camera rotation is turned off and
// the run restarted, so every regeneration shows the same moment; trails are
// parent-relative (absolute trails of moons spiral as Jupiter moves). Only the
// floating panels are hidden; the header stays, so the canvas keeps the
// 640×630 size it is drawn at. bypassCSP: only this generator's page injects a
// style; the published site keeps its strict style-src 'self'.
const sim = await browser.newPage({ viewport: { width: 640, height: 700 }, bypassCSP: true });
await sim.goto(`${base}?body=jupiter`);
await sim.locator('[data-runtime-status]').filter({ hasText: /^Running$/ }).waitFor({ timeout: 90000 });
await sim.waitForFunction(() => /^(\d+)\/\1$/.test(document.querySelector('canvas')?.dataset.textures ?? ''), null, { timeout: 90000 });
const header = await sim.locator('.site-header').evaluate(element => element.getBoundingClientRect().height);
await sim.setViewportSize({ width: 640, height: 630 + Math.round(header) });
await sim.addStyleTag({ content: '.hud,.dock,.inspector,.lesson-strip{display:none!important}' });
const click = selector => sim.evaluate(selector => document.querySelector(selector).click(), selector);
if (await sim.locator('[data-runtime-rotate]').isChecked()) await click('[data-runtime-rotate]');
await click('[data-command="trails"]');
await click('[data-command="reset"]');
await sim.waitForFunction(() => parseFloat(document.querySelector('[data-clock-note]')?.textContent?.replace('+', '') ?? '0') >= 4,
  null, { timeout: 90000, polling: 50 });
await click('[data-command="pause"]');
await click('[data-command="frame"]');
await sim.waitForTimeout(1500);
const frame = (await sim.locator('canvas').screenshot()).toString('base64');
const date = (await sim.locator('[data-runtime-elapsed]').textContent())?.trim();

// The notebook half reuses the site's own fonts, so load it from the site origin.
const card = await browser.newPage({ viewport: { width: 1200, height: 630 }, bypassCSP: true });
await card.goto(`${base}about/`);
await card.setContent(`<!doctype html><html><head><style>
  @font-face { font-family: "Cormorant Garamond"; src: url("${base}fonts/cormorant-garamond-latin.woff2") format("woff2"); font-weight: 500 700; }
  @font-face { font-family: "JetBrains Mono"; src: url("${base}fonts/jetbrains-mono-latin.woff2") format("woff2"); font-weight: 400 700; }
  body { margin: 0; width: 1200px; height: 630px; display: grid; grid-template-columns: 640px 1fr; font-family: system-ui, sans-serif; }
  .frame { position: relative; background: #07090d url(data:image/png;base64,${frame}) center / cover; }
  .clock { position: absolute; left: 28px; top: 24px; font: 500 30px/1.2 "JetBrains Mono"; color: #e7e4dd; text-shadow: 0 1px 4px #000; }
  .clock span { display: block; font-size: 17px; color: #e8aa4e; margin-top: 4px; }
  .paper { background: #f6f1e7; color: #1a2029; padding: 56px 48px 40px; display: flex; flex-direction: column; border-left: 2px solid #1a2029; }
  h1 { font: 700 88px/0.92 "Cormorant Garamond"; margin: 0 0 32px; letter-spacing: -0.01em; }
  .note { border: 2px solid #1a2029; background: #ede7db; padding: 16px 20px; font-size: 25px; line-height: 1.35; }
  .foot { margin-top: auto; padding-top: 14px; border-top: 2px solid #1a2029; display: flex; justify-content: space-between; font: 600 18px "JetBrains Mono"; }
  .foot b { color: #975800; font-weight: 600; }
</style></head><body>
  <div class="frame"><div class="clock">${date}<span>Jupiter and the Galilean moons, live</span></div></div>
  <div class="paper">
    <h1>Solar<br>System<br>Simulator</h1>
    <p class="note">Real N-body physics in C, starting from the sky of 2026-06-09.</p>
    <div class="foot"><span>C11 · raylib · WebAssembly</span><b>Open source</b></div>
  </div>
</body></html>`);
await card.evaluate(() => document.fonts.ready);
writeFileSync(`${publicDir}social-preview.png`, await card.screenshot());

await browser.close();
console.log(`site images written to ${publicDir}`);
