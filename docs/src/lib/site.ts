export const siteName = 'Solar System Simulator';
export const authorName = 'Jonathan Peris';
export const repoUrl = 'https://github.com/jonathanperis/solar-system-simulator';
export const liveUrl = 'https://jonathanperis.github.io/solar-system-simulator/';
export const buildRevision = import.meta.env.PUBLIC_BUILD_SHA || 'local build';

export const currentMilestone = {
  label: 'Guided C experiments, matched A/B comparisons, and the complete pinned small-body atlas',
  shortLabel: 'Learning laboratory',
  integrator: 'velocity-Verlet baseline with explicit Euler lesson comparisons',
  units: 'SI units: m, kg, s, m/s',
  language: 'C11',
  rendering: 'raylib 3D boundary',
  webRuntime: 'Emscripten WebAssembly artifact',
  disclaimer: 'Deterministic educational physics model, not an ephemeris or mission-navigation source.'
};

export const techStack = [
  'C11',
  'raylib',
  'Emscripten',
  'Astro',
  'GitHub Pages',
  'Make',
  'Python smoke checks'
];

export const primaryRoutes = [
  { label: 'Explore', href: 'simulator/', paths: ['', 'simulator/', 'body-catalog/', 'small-bodies/'] },
  { label: 'Learn', href: 'docs/experiments/', paths: ['docs/experiments/', 'physics/'] },
  { label: 'Experiments', href: 'compare/', paths: ['compare/'] },
  { label: 'Reference', href: 'docs/', paths: ['docs/', 'source-atlas/', 'pipeline/'] }
];

export interface DocsRoute {
  label: string;
  href: string;
  summary: string;
  code: string;
  keywords?: string;
}

export interface DocsManualGroup {
  id: string;
  title: string;
  description: string;
  routes: DocsRoute[];
}

export const docsRoutes: DocsRoute[] = [
  { label: 'Docs hub', href: 'docs/', summary: 'Choose a reading path through the simulator.', code: 'MAP' },
  { label: 'Architecture', href: 'docs/architecture/', summary: 'The source boundaries that keep physics testable.', code: 'ARCH' },
  { label: 'Simulation core', href: 'docs/simulation-core/', summary: 'Vectors, bodies, gravity, units, and integration.', code: 'SIM' },
  { label: 'Rendering', href: 'docs/rendering/', summary: 'raylib conversion, scale modes, vector directions, trails, and grids.', code: 'VIEW' },
  { label: 'Controls', href: 'docs/controls/', summary: 'Pause, restart, choose a world, adjust speed, and recover your view.', code: 'KEYS', keywords: 'resume stop play reset zoom camera keyboard touch mobile select search trail vector' },
  { label: 'Build and web', href: 'docs/build-and-web/', summary: 'Native tests, WASM artifacts, Astro, and Pages deployment.', code: 'WASM' },
  { label: 'Roadmap', href: 'docs/roadmap/', summary: 'The one-body-at-a-time expansion model.', code: 'NEXT' },
  { label: 'Guided experiments', href: 'docs/experiments/', summary: 'Predict, run, measure and compare reproducible C lessons.', code: 'LAB' }
];

export const docsManualGroups: DocsManualGroup[] = [
  {
    id: 'MANUAL_01',
    title: 'Start here',
    description: 'Watch your first orbit, choose a question, and get help with the controls.',
    routes: [docsRoutes[7], docsRoutes[4]]
  },
  {
    id: 'MANUAL_02',
    title: 'Physics & view',
    description: 'Trace SI-unit state through integration, rendering scale, vectors, and trails.',
    routes: [docsRoutes[2], docsRoutes[3]]
  },
  {
    id: 'MANUAL_03',
    title: 'Developer reference',
    description: 'Verify native tests, WebAssembly artifacts, Pages deployment, and next bodies.',
    routes: [docsRoutes[1], docsRoutes[5], docsRoutes[6], docsRoutes[0]]
  }
];

export const footerLinks = [
  { label: 'GitHub source', href: repoUrl },
  { label: 'Live Pages lab', href: liveUrl },
  { label: 'Raylib', href: 'https://www.raylib.com/' },
  { label: 'Astro', href: 'https://astro.build/' }
];

export function withBase(base: string, href: string): string {
  if (href.startsWith('http') || href.startsWith('#')) return href;
  return `${base}${href}`;
}
