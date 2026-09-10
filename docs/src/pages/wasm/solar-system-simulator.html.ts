import { normalizeBase } from '../../lib/base';

// A prerendered endpoint preserves the exact historic .html filename on Pages.
// Astro's directory-style redirects would instead emit .html/index.html.
export function GET(): Response {
  const destination = `${normalizeBase(import.meta.env.BASE_URL)}simulator/`;
  return new Response(`<!doctype html>
<html lang="en"><head><meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta http-equiv="refresh" content="0;url=${destination}" />
<link rel="canonical" href="https://jonathanperis.github.io${destination}" />
<title>Live simulator · Solar System Simulator</title></head>
<body><p>The runtime now shares the Astro site. <a href="${destination}">Continue to the simulator</a>.</p></body></html>`,
    { headers: { 'Content-Type': 'text/html; charset=utf-8' } });
}
