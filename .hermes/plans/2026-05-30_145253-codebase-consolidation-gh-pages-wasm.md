# Codebase Consolidation + GitHub Pages WASM Implementation Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task.

**Goal:** Consolidate the current physics-first C/raylib base, add learning-oriented comments/docs, and ship a Super Mango-inspired GitHub Pages site that embeds the simulator as WebAssembly.

**Architecture:** Keep `src/sim/` raylib-independent and SI-unit based, move repeated body/scene metadata toward data-driven helpers only where it reduces duplication, then add a `docs/` Astro site that hosts both source documentation and a base-path-safe WASM canvas. CI should mirror Super Mango's proven structure: native/test/WASM artifact workflow first, Pages deployment second.

**Tech Stack:** C11, raylib, Emscripten, Make, GitHub Actions, Astro 6, Bun, Node 22, GitHub Pages.

---

## Planning Progress Snapshot

**Planning state as of:** 2026-05-30 14:52 UTC  
**Implementation checkpoint:** 2026-05-30 — Tasks 2-5 completed and verified locally
**Repository:** `/opt/data/github/jonathanperis/solar-system-simulator`  
**Branch/ref inspected:** `main @ 015eb1e`  
**Working tree state before planning:** clean and even with `origin/main` (`0 0`)  
**Reference repo inspected:** `/tmp/super-mango-editor @ 184e335` cloned from `jonathanperis/super-mango-editor`  
**Verification during planning:** `make test` passed locally.

| Track | Planned | Evidence checked | Implementation status | Progress |
|---|---:|---:|---|---:|
| Current code audit/orientation | yes | README, Makefile, `src/sim`, `src/render`, `src/app`, tests | audit complete enough for roadmap | 100% |
| Agent/product/design context | yes | Missing `AGENTS.md`, `PRODUCT.md`, `DESIGN.md`; Impeccable context created from repo evidence | planning/context docs created in this pass | 100% |
| Codebase consolidation/refactor | yes | Repeated scene factories, name-based renderer parent/color logic, HUD strings, trail allocation behavior | body IDs/parents, metadata-driven renderer lookup, and generated HUD labels implemented | 70% |
| Commenting/learning docs | yes | sparse source comments, README is detailed but no per-module docs site | source comments added for physics, units, parent-relative moons, trails, renderer compromises, and substeps | 55% |
| WASM build pipeline | yes | current Makefile has native build only; Super Mango `make web` uses Emscripten + controlled startup | roadmap with exact files | 20% |
| GitHub Pages/Astro site | yes | no `docs/`; Super Mango uses Astro `docs/`, base path, loader, deploy workflows | roadmap with exact files | 20% |

**Overall planning progress:** ~60% complete. The audit/context scaffolding and first consolidation pass are complete; implementation remains for native CI, WASM build, docs site, and Pages deployment.

---

## Verified Current Context

### Current solar-system-simulator shape

- `Makefile` builds one native raylib binary and six C test binaries.
- `src/sim/` is cleanly separated from raylib and contains `Vec3d`, units, body model, physics, constants, and scene factories.
- `src/render/renderer.c` owns raylib drawing, render-scale policies, body colors, parent-relative moon visibility, and trail point transforms.
- `src/app/` owns window-independent helpers: full-history body trails and orbit camera math.
- `src/main.c` owns raylib app setup, controls, substep policy, overlay text, camera focus, and app loop.
- No `.github/`, `docs/`, `web/`, `AGENTS.md`, `PRODUCT.md`, or `DESIGN.md` existed before this plan.

### Super Mango structures to borrow, not copy blindly

- Root `AGENTS.md`, `PRODUCT.md`, and `DESIGN.md` keep agent/product/design context close to code.
- `docs/` is an Astro 6 static site with `astro.config.mjs`, `package.json`, `bun.lock`, `src/pages`, `src/components`, `src/layouts`, `src/styles`, and wiki/docs markdown.
- `docs/astro.config.mjs` sets `site: "https://jonathanperis.github.io"` and production `base: "/super-mango-editor"`; this repo should use `base: "/solar-system-simulator"`.
- Build workflow compiles native and WASM artifacts, uploads `*-wasm.zip` as a workflow artifact, then deploy workflow downloads that artifact, builds docs, copies `.js/.wasm/.data` into `docs/out`, smokes the output, and deploys Pages.
- WASM loader uses `data-start-game`, explicit `window.Module`, `locateFile`/asset base awareness, status text, dependency monitor, `onRuntimeInitialized`, and controlled `callMain(...)` startup.
- Docs site is not just markdown: it acts as an inspectable manual with route matrix/source references and a playable browser demo.

---

## Audit Findings and Refactor Opportunities

### P0 — Scene/body metadata is duplicated across sim, render, HUD, tests, and README

Current state:
- Scene factories are named by concatenating body names, e.g. `solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos`.
- Renderer colors and moon parent lookup are name-string conditionals.
- HUD manually repeats the full body list.

Risk:
- Every new planet/moon will require edits in many unrelated places.
- String-based renderer decisions are fragile as body catalog expands.

Refactor direction:
- Add a small `BodyId` enum and optional `parent_id` metadata to `Body`, or add a parallel `SolarBodySpec` table used by factories and renderer helpers.
- Keep the first implementation minimal: no scene file format yet.
- Centralize display color/parent relationship in a catalog so renderer no longer guesses by string.

Acceptance:
- Existing tests still pass.
- New tests prove Phobos/Deimos parent lookup works by metadata, not name comparisons.
- HUD body list can be generated from the `SolarSystem` body names.

### P0 — WebAssembly target is absent

Current state:
- Native raylib build only.
- No `web/` shell, Emscripten target, or browser smoke.

Risk:
- GitHub Pages cannot host the simulator until `make web` exists and works at the project base path.

Refactor/build direction:
- Add `web/shell.html` and `Makefile` targets: `web`, `dist-wasm`, and `wasm-smoke`.
- Use Emscripten raylib guidance; keep native build unchanged.
- Consider `PLATFORM=Web` raylib/Emscripten linking flags and controlled startup. If raylib/Emscripten auto-runs cleanly, document why; otherwise export `_main` and call it from the Astro loader as Super Mango does.

Acceptance:
- `make web` creates `build/web/solar-system-simulator.js` and `.wasm` (and `.data` only if assets are introduced later).
- Local artifact smoke checks that files exist and the generated JS references the WASM.

### P1 — Source comments need learning-focused coverage

Current state:
- README is detailed, but code has very few comments.
- Key concepts such as velocity-Verlet, vis-viva, units, parent-relative moon initialization, renderer-only illustrative offsets, trail growth, and app substeps deserve explanatory comments.

Risk:
- The project is harder to learn from and future agents may refactor away important physics boundaries.

Commenting direction:
- Add short comments above non-obvious functions/blocks.
- Avoid obvious comments like "increment i".
- Add module header comments to explain responsibility and boundaries.

Acceptance:
- Each major source file has intent comments.
- Comments explain units/approximations/tradeoffs where relevant.
- `git diff --check` and `make test` pass.

### P1 — Docs site should be a space-themed manual and playable lab

Current state:
- README is the only user docs surface.
- No docs route matrix or source atlas.

Direction:
- Create `docs/` Astro site with a space/observatory aesthetic from `DESIGN.md`.
- Pages:
  - `/` Observatory deck + WASM viewport + body manifest + source/pipeline portals.
  - `/docs/` Manual command center.
  - `/docs/physics/` SI units, Newtonian gravity, integrator, vis-viva.
  - `/docs/source/` module/source atlas.
  - `/docs/pipeline/` native/test/WASM/Pages pipeline explanation.
  - `/docs/bodies/` current body catalog and roadmap.

Acceptance:
- `cd docs && bun install && bun run build` succeeds.
- Site uses production base path `/solar-system-simulator`.
- Home page includes a canvas area and loader status even before WASM artifacts are available.

### P2 — Pipeline and docs drift checks can be introduced gradually

Current state:
- No CI workflows.
- Tests run locally via `make test`.

Direction:
- Add `.github/workflows/build.yml` for native C/raylib checks and WASM artifact build.
- Add `.github/workflows/docs.yml` for docs lint/build on PR/manual.
- Add `.github/workflows/deploy.yml` for Pages deploy after successful build.
- Add comments to workflows explaining each stage.

Acceptance:
- Workflows are commented and minimal.
- Pages deploy uses GitHub Pages permissions and uploads `docs/out`.
- Build workflow uploads a named WASM artifact consumed by deploy workflow.

---

## Implementation Tasks

### Task 1: Commit planning/context scaffold

**Objective:** Preserve this audit and future-agent context before implementation begins.

**Files:**
- Create: `AGENTS.md`
- Create: `PRODUCT.md`
- Create: `DESIGN.md`
- Create: `.hermes/plans/2026-05-30_145253-codebase-consolidation-gh-pages-wasm.md`

**Steps:**
1. Run `git diff --check`.
2. Run `make test`.
3. Commit:
   ```bash
   git add AGENTS.md PRODUCT.md DESIGN.md .hermes/plans/2026-05-30_145253-codebase-consolidation-gh-pages-wasm.md
   git commit -m "docs: add consolidation and pages plan"
   git push origin main
   ```

**Expected:** clean diff check, all six tests pass, pushed commit on `main`.

---

### Task 2: Add body identity/catalog metadata without changing behavior

**Objective:** Reduce future duplication by giving bodies stable IDs and parent relationships.

**Files:**
- Modify: `src/sim/body.h`
- Modify: `src/sim/body.c`
- Modify: `src/sim/solar_system.c`
- Modify: `src/sim/solar_system.h`
- Test: `tests/test_solar_system.c`

**Step 1: RED tests**
- Add assertions that each current scene body has a stable ID: Sun, Mercury, Venus, Earth, Moon, Mars, Phobos, Deimos.
- Add assertions that Moon parent is Earth, Phobos/Deimos parent is Mars, planets have no parent.

**Step 2: Implement minimally**
- Add:
  ```c
  typedef enum BodyId {
      BODY_ID_SUN,
      BODY_ID_MERCURY,
      BODY_ID_VENUS,
      BODY_ID_EARTH,
      BODY_ID_MOON,
      BODY_ID_MARS,
      BODY_ID_PHOBOS,
      BODY_ID_DEIMOS,
      BODY_ID_NONE
  } BodyId;
  ```
- Add `BodyId id; BodyId parent_id;` to `Body`.
- Update `body_create(...)` signature or add `body_create_with_metadata(...)` while keeping call sites simple.

**Step 3: Verify**
```bash
make build/tests/test_solar_system && build/tests/test_solar_system
make test
```

**Acceptance:** no render/app behavior changes; tests prove metadata exists.

---

### Task 3: Replace renderer string lookups with metadata-driven helpers

**Objective:** Make renderer expansion safer before adding many more bodies.

**Files:**
- Modify: `src/render/renderer.c`
- Test: `tests/test_renderer.c`

**Step 1: RED tests**
- Add a test that parent lookup for Phobos/Deimos is based on `parent_id` by using expected metadata, not name-only assumptions.
- Add a test that each current `BodyId` maps to a documented color.

**Step 2: Implement minimally**
- Replace `body_named(...)` / `moon_parent_index(...)` logic with parent metadata lookup.
- Replace name-based color conditionals with `switch (body->id)`.

**Step 3: Verify**
```bash
make build/tests/test_renderer && build/tests/test_renderer
make test
```

**Acceptance:** real-scale vs illustrative behavior remains unchanged, but no current renderer rule depends on body name strings.

---

### Task 4: Generate HUD/body-list strings from system state

**Objective:** Remove repeated full body lists from `src/main.c` before the catalog grows.

**Files:**
- Modify: `src/main.c`
- Optional create/modify: `src/app/body_labels.c`, `src/app/body_labels.h`
- Test: new `tests/test_body_labels.c` if helper is extracted
- Modify: `Makefile` if adding a new test binary

**Step 1: RED test**
- If extracting helper, assert that the current 8-body scene produces `Sun + Mercury + Venus + Earth + Moon + Mars + Phobos + Deimos`.

**Step 2: Implement minimally**
- Add a small fixed-buffer helper for display labels.
- Keep allocation out of the frame loop if practical.

**Step 3: Verify**
```bash
make test
make
```

**Acceptance:** HUD still displays the current bodies, and adding a future body requires fewer manual string edits.

---

### Task 5: Add learning comments to current C code

**Objective:** Comment the code for learners without bloating obvious syntax.

**Files:**
- Modify: `src/sim/*.c`, `src/sim/*.h`
- Modify: `src/app/*.c`, `src/app/*.h`
- Modify: `src/render/*.c`, `src/render/*.h`
- Modify: `src/main.c`

**Comment targets:**
- SI-unit boundaries in `Body`, `Vec3d`, and `units`.
- Newtonian acceleration and velocity-Verlet in `physics.c`.
- Vis-viva/perihelion/perigee/periareion assumptions in `solar_system.c`.
- Trail dynamic growth and why trails live outside simulation.
- Orbit camera fixed-pitch/zoom clamp.
- Render-only illustrative radius and parent-relative moon separation.
- App substeps for short-period moons.

**Verification:**
```bash
git diff --check
make test
```

**Acceptance:** comments improve learning and preserve all behavior.

---

### Task 6: Add native CI workflow

**Objective:** Give GitHub a baseline build/test gate before Pages deployment.

**Files:**
- Create: `.github/workflows/build.yml`

**Implementation shape:**
- Trigger on `push` to `main`, `pull_request`, and `workflow_dispatch`.
- Linux native job:
  - checkout;
  - install `build-essential`, `pkg-config`, `cmake`, raylib dependencies if using package raylib, or build raylib headless if apt package is insufficient;
  - run `make clean && make && make test`.
- Add comments explaining each phase.

**Verification:**
```bash
git diff --check
```

**Acceptance:** workflow is readable, minimal, and does not deploy anything yet.

---

### Task 7: Add WebAssembly Makefile target and shell

**Objective:** Build browser artifacts locally/CI via Emscripten.

**Files:**
- Modify: `Makefile`
- Create: `web/shell.html`
- Optional create: `tools/check_wasm_artifacts.py`

**Implementation shape:**
- Add variables:
  ```make
  WEB_DIR := build/web
  WEB_APP := $(WEB_DIR)/solar-system-simulator.html
  ```
- Add `web` target using `emcc` with raylib's web platform flags.
- Add `dist-wasm` target to zip `.html`, `.js`, `.wasm`, and `.data` if present.
- Add comments explaining Emscripten/raylib requirements.

**Verification:**
```bash
make web
python3 tools/check_wasm_artifacts.py
```

**Acceptance:** browser artifacts are produced and smoke-checked. If local Emscripten is not installed, CI task should still document/setup it.

---

### Task 8: Extend build workflow with WASM artifact upload

**Objective:** Produce a Pages-consumable WASM artifact like Super Mango.

**Files:**
- Modify: `.github/workflows/build.yml`

**Implementation shape:**
- Add WebAssembly matrix leg or separate `wasm` job.
- Use `mymindstorm/setup-emsdk`.
- Run `emcc --version`, `make web`, `make dist-wasm` or zip manually.
- Upload artifact named `solar-system-simulator-wasm`.

**Acceptance:** build workflow contains a native test path and a WASM artifact path, clearly commented.

---

### Task 9: Create Astro docs site skeleton

**Objective:** Add a static GitHub Pages docs site with space aesthetic foundations.

**Files:**
- Create: `docs/package.json`
- Create: `docs/astro.config.mjs`
- Create: `docs/tsconfig.json`
- Create: `docs/public/.nojekyll`
- Create: `docs/src/layouts/BaseLayout.astro`
- Create: `docs/src/pages/index.astro`
- Create: `docs/src/pages/docs/index.astro`
- Create: `docs/src/styles/global.css`

**Implementation shape:**
- Astro config:
  ```js
  site: "https://jonathanperis.github.io",
  base: isProd ? "/solar-system-simulator" : "",
  output: "static",
  outDir: "out"
  ```
- Use Node 22-compatible Astro 6 dependencies.
- Apply `DESIGN.md` palette: dark space, observatory panels, solar/cyan accents.

**Verification:**
```bash
cd docs
bun install
bun run build
```

**Acceptance:** docs site builds and has home/docs routes.

---

### Task 10: Add WASM viewport component and loader

**Objective:** Embed the simulator in Pages with explicit status and base-path-safe asset loading.

**Files:**
- Create: `docs/src/components/WasmObservatory.astro`
- Modify: `docs/src/pages/index.astro`
- Modify: `docs/src/styles/global.css`

**Implementation shape:**
- Canvas section with `id="canvas"` and status element.
- Buttons: normal run and debug/diagnostic run if supported by C app later.
- `data-asset-base={`${base}/`}` style asset base from `import.meta.env.BASE_URL`.
- `window.Module` with `canvas`, `locateFile`, `print`, `printErr`, `setStatus`, `monitorRunDependencies`, and runtime error handling.
- Load `solar-system-simulator.js` from asset base.

**Verification:**
```bash
cd docs && bun run build
python3 -m http.server 4173 --directory out
# curl home and expected artifact URLs after copying sample/built artifacts
```

**Acceptance:** no blank-canvas failure mode; status explains missing or loading artifacts.

---

### Task 11: Add source atlas and physics docs routes

**Objective:** Turn GitHub Pages into a learning manual, not just a demo.

**Files:**
- Create: `docs/src/pages/docs/physics.astro` or markdown route
- Create: `docs/src/pages/docs/source.astro`
- Create: `docs/src/pages/docs/bodies.astro`
- Create: `docs/src/pages/docs/pipeline.astro`

**Content requirements:**
- Explain `src/sim`, `src/render`, `src/app`, and `src/main.c` responsibilities.
- Include formulas for acceleration and vis-viva.
- Link to source files on GitHub.
- Document native commands and future WASM/Pages commands.
- Comment every major code/pipeline concept in prose.

**Verification:**
```bash
cd docs && bun run build
```

**Acceptance:** docs section teaches code structure and physics assumptions clearly.

---

### Task 12: Add docs workflow

**Objective:** Validate docs independently on PR/manual runs.

**Files:**
- Create: `.github/workflows/docs.yml`

**Implementation shape:**
- Trigger on docs/source/workflow changes.
- Checkout, setup Node 22, setup Bun, install docs deps, run `bun run build`.
- Add comments similar to Super Mango explaining each step.

**Acceptance:** docs build path is independent of Pages deployment.

---

### Task 13: Add Pages deploy workflow

**Objective:** Deploy docs + WASM artifact to GitHub Pages after successful build.

**Files:**
- Create: `.github/workflows/deploy.yml`

**Implementation shape:**
- Trigger on successful `Build` workflow completion on `main`.
- Permissions: `pages: write`, `id-token: write`, `contents: read`, `actions: read`.
- Download artifact `solar-system-simulator-wasm` from triggering run.
- Build docs.
- Copy `.js`, `.wasm`, `.data` if present into `docs/out/`.
- Smoke:
  - `test -s docs/out/index.html`
  - `test -s docs/out/solar-system-simulator.js`
  - `test -s docs/out/solar-system-simulator.wasm`
  - local `python3 -m http.server` + `curl` for `/`, `/solar-system-simulator.js`, `/solar-system-simulator.wasm`.
- Upload/deploy Pages artifact.

**Acceptance:** mirrors Super Mango's artifact-then-deploy pattern with this repo's names/base path.

---

### Task 14: Browser verification and polish pass

**Objective:** Prove the final site works and meets the space/observatory aesthetic.

**Files:**
- Modify docs/CSS/components as needed.

**Verification:**
```bash
make clean && make && make test
make web
cd docs && bun run build && bun run preview
```

**Browser checks:**
- Home page loads at `/solar-system-simulator/` base path in local preview or deployed Pages.
- Normal run requests `solar-system-simulator.js` and `.wasm` from the correct base path.
- Canvas shows the raylib simulator or an actionable error.
- Docs root and representative docs pages render.
- Run Impeccable detect/polish on changed UI surfaces if available.

**Acceptance:** implementation is verified locally and, after push, by GitHub Actions/Pages.

---

## Validation Gates

Before final report for implementation:

```bash
git diff --check
make clean && make && make test
make web
cd docs && bun install && bun run build
```

After push:

- Inspect GitHub Actions build/docs/deploy runs.
- Confirm Pages URL: `https://jonathanperis.github.io/solar-system-simulator/`.
- Browser-smoke the WASM loader and docs routes.

---

## Risks and Tradeoffs

- **raylib + Emscripten flags may differ from SDL2 Super Mango.** Treat Super Mango as structural inspiration, not a copy-paste source.
- **Browser main-loop lifetime:** if the C app passes stack-owned long-lived state into an Emscripten main loop, move app state to heap or a stable static lifetime before release.
- **Base path bugs:** always verify under `/solar-system-simulator/`, not only at `/`.
- **Comment bloat:** comments should teach physics/boundaries, not narrate obvious syntax.
- **Catalog refactor scope:** keep metadata minimal; do not introduce a full scene format until future body count makes it necessary.

---

## Recommended Execution Order

1. Commit this plan/context scaffold.
2. Do metadata/catalog refactors and comments first while native behavior is stable.
3. Add native CI.
4. Add WASM build and artifact smoke.
5. Add Astro docs shell and space aesthetic.
6. Add loader and docs routes.
7. Add docs/deploy workflows.
8. Push, monitor Actions, and verify live Pages.
