/**
 * Content-Security-Policy for every generated page.
 *
 * The site ships no inline scripts, inline style elements or style attributes:
 * Astro emits bundled modules/stylesheets as files (see astro.config.mjs), the
 * atlas positions come from a generated stylesheet, and analytics bootstraps
 * from a static file. That keeps the policy free of hashes and 'unsafe-inline'.
 *
 * - 'wasm-unsafe-eval' permits WebAssembly compilation (simulator, comparison
 *   lab, catalog orbit kernel) without allowing JavaScript eval.
 * - worker-src 'self' covers the bundled catalog search worker.
 * - Downloads use blob: object URLs on anchor navigation, which fetch
 *   directives do not govern, so no blob: source is needed.
 * - Google Analytics hosts are allow-listed only when the build carries
 *   PUBLIC_GA_ID (deployed main, V17); local and fork builds contact no third party.
 *
 * Delivered as a <meta> tag because GitHub Pages cannot set response headers;
 * meta delivery ignores frame-ancestors/report-uri, so they are omitted.
 */
export const analyticsHosts = {
  script: ['https://www.googletagmanager.com'],
  connect: ['https://*.google-analytics.com', 'https://*.analytics.google.com', 'https://*.googletagmanager.com'],
  img: ['https://*.google-analytics.com', 'https://*.googletagmanager.com']
} as const;

export function contentSecurityPolicy(analytics: boolean): string {
  const extra = (hosts: readonly string[]) => analytics ? hosts : [];
  const directives: [string, ...string[]][] = [
    ['default-src', "'self'"],
    ['script-src', "'self'", "'wasm-unsafe-eval'", ...extra(analyticsHosts.script)],
    ['style-src', "'self'"],
    ['img-src', "'self'", ...extra(analyticsHosts.img)],
    ['font-src', "'self'"],
    ['connect-src', "'self'", ...extra(analyticsHosts.connect)],
    ['worker-src', "'self'"],
    ['object-src', "'none'"],
    ['base-uri', "'self'"],
    ['form-action', "'self'"]
  ];
  return directives.map(parts => parts.join(' ')).join('; ');
}
