import { liveUrl } from '../lib/site';

// Every canonical page is an Astro page in this directory. Deriving the list
// from the page modules keeps the sitemap complete when routes are added; the
// legacy compatibility redirect is deliberately excluded (it is noindex).
const sitemapRoutes = Object.keys(import.meta.glob('./**/*.astro'))
  .filter(file => !file.startsWith('./wasm/'))
  .map(file => file.slice(2, -'.astro'.length).replace(/(^|\/)index$/, ''))
  .map(route => (route ? `${route}/` : ''))
  .sort();

export function GET(): Response {
  const urls = sitemapRoutes.map(route => `  <url><loc>${liveUrl}${route}</loc></url>`).join('\n');
  return new Response(`<?xml version="1.0" encoding="UTF-8"?>
<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
${urls}
</urlset>
`, { headers: { 'Content-Type': 'application/xml; charset=utf-8' } });
}
