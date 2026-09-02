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
    responsibility: 'orbit camera state, bounded full-run trails, and other window-independent app helpers.',
    verification: 'C tests exercise zoom clamps, trail history endpoints, and bounded trail storage.'
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
    label: 'Web shell',
    path: 'web/shell.html',
    responsibility: 'Emscripten HTML shell that hosts the C/raylib WebAssembly artifact.',
    verification: 'Checked by tools/check_wasm_artifacts.py during WebAssembly builds.'
  },
  {
    label: 'Automation',
    path: '.github/workflows/',
    responsibility: 'Build workflow for native and WASM gates, plus workflow-run Pages deployment.',
    verification: 'GitHub Actions Build and Deploy Pages runs must pass for the pushed SHA.'
  }
];
