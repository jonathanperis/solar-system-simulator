export type SourceSection = {
  label: string;
  path: string;
  responsibility: string;
  verification: string;
};

export const sourceSections: SourceSection[] = [
  {
    label: 'Project spec',
    path: 'SPEC.md',
    responsibility: 'Current goals, constraints, interfaces, invariants, tasks, and bug history.',
    verification: 'Review before an implementation task; check drift after verification.'
  },
  {
    label: 'Simulation core',
    path: 'src/sim/',
    responsibility: 'raylib-independent bodies, constants, Vec3d math, Newtonian acceleration, Verlet stepping, and scene factories.',
    verification: 'C tests cover vector math, acceleration, body initialization, and time stepping.'
  },
  {
    label: 'Application helpers',
    path: 'src/app/',
    responsibility: 'C-owned playback/selection session, physical inspector, orbit camera, and bounded full-run trails.',
    verification: 'C tests exercise pause/reset/single-step, parent-relative inspection, camera framing, 100-day accuracy, and curved trail coverage.'
  },
  {
    label: 'Rendering boundary',
    path: 'src/render/',
    responsibility: 'raylib conversion from SI-unit simulation state into readable 3D drawing policies.',
    verification: 'Renderer helper tests guard scale conversion, visual radius policy, family framing, and grid sizing.'
  },
  {
    label: 'Runtime loop',
    path: 'src/main.c',
    responsibility: 'native window loop, Emscripten callback loop, input handling, simulation stepping, and draw orchestration.',
    verification: 'Native and WebAssembly builds compile both loop targets; artifact checks validate generated files.'
  },
  {
    label: 'Test binaries',
    path: 'tests/',
    responsibility: 'focused C binaries for physics and app behavior without requiring a desktop window.',
    verification: 'Run with make test.'
  },
  {
    label: 'Public site',
    path: 'docs/',
    responsibility: 'Astro Pages site, documentation routes, public lab shell, and source-backed explanatory content.',
    verification: 'Run npm run build --prefix docs and the docs route smoke checker.'
  },
  {
    label: 'Browser runtime',
    path: 'docs/src/pages/simulator.astro',
    responsibility: 'Shared Astro layout, controls, and canvas; simulator.ts sends commands to C and presents live physical readouts and errors.',
    verification: 'Runtime integration tests, generated route checks, and headless browser verification.'
  },
  {
    label: 'Automation',
    path: '.github/workflows/',
    responsibility: 'Build workflow for native and WASM gates, plus workflow-run Pages deployment.',
    verification: 'GitHub Actions Build and Deploy Pages runs must pass for the pushed SHA.'
  }
];
