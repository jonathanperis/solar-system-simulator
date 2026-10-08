export const siteName = 'Solar System Simulator';
export const authorName = 'Jonathan Peris';
export const repoUrl = 'https://github.com/jonathanperis/solar-system-simulator';
export const liveUrl = 'https://jonathanperis.github.io/solar-system-simulator/';
export const buildRevision = import.meta.env.PUBLIC_BUILD_SHA || 'local build';

/** The four routes of the site (DESIGN.md, SPEC A99). The simulator is home. */
export const primaryRoutes = [
  { label: 'Learn', href: 'learn/' },
  { label: 'Catalog', href: 'catalog/' },
  { label: 'About', href: 'about/' }
];

/** Every retired URL forwards to its new home, keeping `?body=` and the
 * fragment (see public/scripts/forward.js). Rendered by pages/[...legacy].astro;
 * the historic Emscripten file is pages/wasm/solar-system-simulator.html.astro. */
export const legacyRedirects: { from: string; to: string; label: string }[] = [
  { from: 'simulator', to: '', label: 'the simulator' },
  { from: 'compare', to: 'learn/compare/', label: 'Compare two experiments' },
  { from: 'body-catalog', to: 'catalog/', label: 'the catalog' },
  { from: 'small-bodies', to: 'catalog/small-bodies/', label: 'the small-body atlas' },
  { from: 'physics', to: 'about/#model', label: 'About: the model' },
  { from: 'source-atlas', to: 'about/#code', label: 'About: the code' },
  { from: 'pipeline', to: 'about/#build', label: 'About: build and tests' },
  { from: 'docs', to: 'about/', label: 'About' },
  { from: 'docs/architecture', to: 'about/#code', label: 'About: the code' },
  { from: 'docs/simulation-core', to: 'about/#model', label: 'About: the model' },
  { from: 'docs/rendering', to: 'about/#rendering', label: 'About: rendering' },
  { from: 'docs/controls', to: 'about/#controls', label: 'About: controls' },
  { from: 'docs/build-and-web', to: 'about/#build', label: 'About: build and tests' },
  { from: 'docs/roadmap', to: 'about/#roadmap', label: 'About: what is next' },
  { from: 'docs/experiments', to: 'learn/', label: 'Learn' }
];

export function withBase(base: string, href: string): string {
  if (href.startsWith('http') || href.startsWith('#')) return href;
  return `${base}${href}`;
}
