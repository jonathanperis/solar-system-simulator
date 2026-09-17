export type SourceSection = {
  label: string;
  path: string;
  responsibility: string;
  verification: string;
};

export const sourceSections: SourceSection[] = [
  {
    label: 'Small-body atlas and source epoch',
    path: 'docs/public/catalog/ + data/planet_epoch.json',
    responsibility: 'Pinned shard/index/density inventory, physical-data supplements in data/, and planetary vectors for bounded selected-body experiments. Provenance lives in data/README.md and data/SMALL_BODIES.md.',
    verification: 'Offline tools/small_body_catalog.py --check and tools/planet_epoch.py --check; conic/experiment C tests and catalog-worker tests.'
  },
  {
    label: 'Jovian satellite catalog',
    path: 'data/jovian_moons.json',
    responsibility: 'Versioned source elements, stable JPL codes, physical-data quality and references for all 115 Jupiter moons; shared with C and Astro.',
    verification: 'Offline tools/jovian_catalog.py --check, C satellite/geometry tests, and full-scene convergence.'
  },
  {
    label: 'Project spec',
    path: 'SPEC.md',
    responsibility: 'Current goals, constraints, interfaces, invariants, tasks, and bug history.',
    verification: 'Review before an implementation task; check drift after verification.'
  },
  {
    label: 'Simulation core',
    path: 'src/sim/',
    responsibility: 'raylib-independent SI bodies, Newtonian forces, Verlet/Euler stepping, diagnostics, lessons, opt-in head-on contact, conic propagation, and scene factories.',
    verification: 'make test-core covers initial states, orbital references, convergence, conservation, collisions, and source-catalog contracts without raylib.'
  },
  {
    label: 'Application helpers',
    path: 'src/app/',
    responsibility: 'C-owned sessions, fixed-step playback, physical inspection, camera, synchronized bounded trails, shared SI CSV writer, versioned lesson descriptors, and matched A/B comparison checkpoints.',
    verification: 'C tests exercise reset/step, inspection, camera framing, 100-day accuracy, trail retention, descriptor validation, and comparison timing/identity.'
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
    label: 'Headless and comparison entrypoints',
    path: 'src/headless.c + src/lab_web.c',
    responsibility: 'The raylib-free solar-lab CLI and the C-only comparison WASM module share sessions, descriptors, measurements, and CSV export. examples/ holds replayable .solar inputs.',
    verification: 'make test-cli; tools/test_learning_wasm.mjs compares matching native/WASM experiments.'
  },
  {
    label: 'Test binaries',
    path: 'tests/',
    responsibility: 'Focused C binaries plus Python CLI, build-graph, catalog-import, and artifact-validation tests; no desktop window is required.',
    verification: 'make test test-build test-cli test-validators; make test-sanitize for address/undefined-behavior checks.'
  },
  {
    label: 'Public site',
    path: 'docs/',
    responsibility: 'Astro Pages site, documentation routes, public lab shell, and source-backed explanatory content.',
    verification: 'npm test --prefix docs, npm run check --prefix docs, npm run build --prefix docs, and make docs-check; docs/browser-tests/ covers browser interaction.'
  },
  {
    label: 'Catalog and delivery tools',
    path: 'tools/',
    responsibility: 'Explicit source refresh/import, offline audits, C/WASM parity checks, runtime artifact provenance/staging, generated-route validation, and the loopback site server.',
    verification: 'make test-build test-validators, offline catalog checks, WASM checks, and make docs-check.'
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
    responsibility: 'Read-only Build gates produce a checked Pages tree; trusted workflow-run deployment publishes that exact artifact. CodeQL separately analyzes C/C++, JavaScript/TypeScript, and Actions.',
    verification: 'Inspect Build/CodeQL for the revision and, on eligible main delivery, Deploy Pages plus the public runtime manifest.'
  }
];
