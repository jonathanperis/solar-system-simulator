export const siteName = 'Solar System Simulator';
export const authorName = 'Jonathan Peris';
export const repoUrl = 'https://github.com/jonathanperis/solar-system-simulator';
export const liveUrl = 'https://jonathanperis.github.io/solar-system-simulator/';

export const currentMilestone = {
  label: 'Foundation, Sun, inner planets, Earth Moon system, Mars moons',
  shortLabel: 'Milestone 7',
  implementedBodyCount: 8,
  integrator: 'velocity-Verlet / kick-drift-kick',
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
  { label: 'Lab', href: '' },
  { label: 'Docs', href: 'docs/' },
  { label: 'Physics', href: 'physics/' },
  { label: 'Bodies', href: 'body-catalog/' },
  { label: 'Source', href: 'source-atlas/' },
  { label: 'Pipeline', href: 'pipeline/' }
];

export interface DocsRoute {
  label: string;
  href: string;
  summary: string;
  code: string;
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
  { label: 'Rendering', href: 'docs/rendering/', summary: 'raylib conversion, scale modes, labels, trails, and grids.', code: 'VIEW' },
  { label: 'Controls', href: 'docs/controls/', summary: 'Camera focus, zoom, view modes, and runtime keys.', code: 'KEYS' },
  { label: 'Build and web', href: 'docs/build-and-web/', summary: 'Native tests, WASM artifacts, Astro, and Pages deployment.', code: 'WASM' },
  { label: 'Roadmap', href: 'docs/roadmap/', summary: 'The one-body-at-a-time expansion model.', code: 'NEXT' }
];

export const docsManualGroups: DocsManualGroup[] = [
  {
    id: 'MANUAL_01',
    title: 'Start here',
    description: 'Orient the repo, source boundaries, controls, and first simulator run.',
    routes: [docsRoutes[0], docsRoutes[1], docsRoutes[4]]
  },
  {
    id: 'MANUAL_02',
    title: 'Physics & view',
    description: 'Trace SI-unit state through integration, rendering scale, labels, and trails.',
    routes: [docsRoutes[2], docsRoutes[3]]
  },
  {
    id: 'MANUAL_03',
    title: 'Build & roadmap',
    description: 'Verify native tests, WebAssembly artifacts, Pages deployment, and next bodies.',
    routes: [docsRoutes[5], docsRoutes[6]]
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
