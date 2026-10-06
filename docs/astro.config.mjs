import { defineConfig } from 'astro/config';
import { readdir, rename, rmdir } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';

/**
 * Astro's directory build format renders `src/pages/x.html.astro` to
 * `x.html/index.html`. Historic public URLs need the exact `x.html` file, so
 * after the build this integration moves each such Astro-rendered document into
 * place. The page markup still comes from the Astro page and shared layout; only
 * its output path changes. Route validation then requires the Astro generator
 * marker on every published .html file.
 */
const legacyHtmlRoutes = ['wasm/solar-system-simulator.html'];
function legacyHtmlRouteFiles() {
  return {
    name: 'legacy-html-route-files',
    hooks: {
      'astro:build:done': async ({ dir }) => {
        const root = fileURLToPath(dir);
        for (const route of legacyHtmlRoutes) {
          const folder = `${root}${route}`;
          const entries = await readdir(folder);
          if (entries.length !== 1 || entries[0] !== 'index.html') throw new Error(`Unexpected legacy route output in ${route}`);
          await rename(`${folder}/index.html`, `${folder}.tmp`);
          await rmdir(folder);
          await rename(`${folder}.tmp`, folder);
        }
      }
    }
  };
}

export default defineConfig({
  site: 'https://jonathanperis.github.io',
  base: '/solar-system-simulator',
  output: 'static',
  integrations: [legacyHtmlRouteFiles()],
  // Emit every stylesheet and processed script as a file. With no inline
  // <style>/<script> content, the page CSP (src/lib/csp.ts) can use plain
  // 'self' sources without hashes or 'unsafe-inline'.
  build: { inlineStylesheets: 'never' },
  vite: { build: { assetsInlineLimit: 0 } }
});
