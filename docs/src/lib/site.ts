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

export const docsRoutes = [
  { label: 'Docs hub', href: 'docs/', summary: 'Choose a reading path through the simulator.' },
  { label: 'Architecture', href: 'docs/architecture/', summary: 'The source boundaries that keep physics testable.' },
  { label: 'Simulation core', href: 'docs/simulation-core/', summary: 'Vectors, bodies, gravity, units, and integration.' },
  { label: 'Rendering', href: 'docs/rendering/', summary: 'raylib conversion, scale modes, labels, trails, and grids.' },
  { label: 'Controls', href: 'docs/controls/', summary: 'Camera focus, zoom, view modes, and runtime keys.' },
  { label: 'Build and web', href: 'docs/build-and-web/', summary: 'Native tests, WASM artifacts, Astro, and Pages deployment.' },
  { label: 'Roadmap', href: 'docs/roadmap/', summary: 'The one-body-at-a-time expansion model.' }
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
