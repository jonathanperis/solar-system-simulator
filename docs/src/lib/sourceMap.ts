export type SourceSection = {
  label: string;
  path: string;
  responsibility: string;
  verification: string;
};

export const sourceSections: SourceSection[] = [
  {
    label: 'Simulation core',
    path: 'src/sim/',
    responsibility: 'raylib-independent bodies, constants, Vec3d math, Newtonian acceleration, Verlet stepping, and scene factories.',
    verification: 'C tests cover vector math, acceleration, body initialization, and time stepping.'
  },
  {
    label: 'Application helpers',
    path: 'src/app/',
    responsibility: 'orbit camera state, body trails, labels, and other window-independent app helpers.',
    verification: 'C tests exercise camera focus, zoom clamps, labels, and trail history.'
  },
  {
    label: 'Rendering boundary',
    path: 'src/render/',
    responsibility: 'raylib conversion from SI-unit simulation state into readable 3D drawing policies.',
    verification: 'Renderer helper tests guard scale conversion, visual radius policy, and grid sizing.'
  },
  {
    label: 'Runtime loop',
    path: 'src/main.c',
    responsibility: 'native window loop, Emscripten callback loop, input handling, simulation stepping, and draw orchestration.',
    verification: 'Native build, web build, and smoke checks prove both loop targets compile.'
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
    label: 'Web shell',
    path: 'web/shell.html',
    responsibility: 'Emscripten HTML shell that hosts the C/raylib WebAssembly artifact.',
    verification: 'Checked by tools/check_wasm_artifacts.py and live Pages smoke tests.'
  },
  {
    label: 'Automation',
    path: '.github/workflows/',
    responsibility: 'Build workflow for native and WASM gates, plus workflow-run Pages deployment.',
    verification: 'GitHub Actions Build and Deploy Pages runs must pass for the pushed SHA.'
  }
];
