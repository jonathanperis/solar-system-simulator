# Solar System Simulator — AGENTS Guide

A physics-first 3D solar system simulator written in C11 with raylib. This repository is intentionally small, educational, and expansion-oriented: every new body should improve the physical model, tests, and documentation without turning the project into a graphics-first engine.

---

## Repository identity

- **Language:** C11 only for simulator/runtime code.
- **Graphics/windowing:** raylib.
- **Architecture:** deterministic SI-unit simulation isolated from rendering.
- **Current scene:** Sun, Mercury, Venus, Earth, Moon, Mars, Phobos, Deimos.
- **Primary goal:** teach and verify orbital mechanics foundations before visual polish.
- **Current public-site direction:** cartoon cosmic cockpit, source-backed and playful, with friendly planet art wrapped around SI-unit physics.

---

## Build and verification commands

```sh
make        # build the native raylib app at build/solar-system-simulator
make test   # run all C test binaries
make run    # launch the simulator locally
make clean  # remove build outputs
```

Use `make test` for every simulation/app/render-helper change. For bigger milestones, run `make clean && make && make test` before committing.

---

## Project structure

```text
solar-system-simulator/
├── AGENTS.md              # maintainer/agent guide
├── PRODUCT.md             # product and learning context for Impeccable/design work
├── DESIGN.md              # cartoon cosmic cockpit visual direction for current GitHub Pages work
├── Makefile               # native build/test entrypoint
├── README.md              # user-facing project status and physics notes
├── .hermes/plans/         # committed implementation plans for this repo
├── src/
│   ├── main.c             # raylib app loop, camera controls, overlays, simulation stepping
│   ├── app/               # app-owned helpers testable without opening a window
│   │   ├── body_trails.*  # persistent motion-history storage for non-star bodies
│   │   └── orbit_camera.* # small stable orbit camera math
│   ├── render/            # raylib presentation boundary and render-scale policy
│   └── sim/               # raylib-independent physics/data model in SI units
└── tests/                 # C test binaries for math, physics, scenes, app helpers, renderer helpers
```

---

## Architectural rules

1. **Keep physics and rendering separate**
   - `src/sim/` must not include raylib headers or use raylib `Vector3`.
   - Simulation state uses meters, kilograms, seconds, meters/second, and meters/second².
   - Convert to raylib types only in `src/render/` or `src/main.c`.

2. **Preserve true physical data**
   - Do not resize bodies, alter positions, or change velocities to make visuals prettier.
   - If the scene needs readability, add explicit renderer/app presentation policy and tests.

3. **Expand one milestone at a time**
   - Add constants, factory/init logic, tests, renderer visibility, app/HUD integration, README/docs, then verify.
   - Keep old scene factories when they help tests preserve earlier milestones.

4. **Use physically named helpers**
   - Prefer names like `*_perihelion`, `*_perigee`, `*_periareion`, `*_speed_mps`, and `*_distance_m`.
   - Comment non-obvious orbital assumptions and source/units for constants.

5. **Tests before implementation for new behavior**
   - Add RED tests for constants, initial state, parent-relative moon invariants, app helpers, renderer transforms, or pipeline outputs.
   - Then implement the smallest change that makes those tests pass.

6. **Avoid broad rewrites**
   - Refactor surgically around repeated body creation, render lookup, or docs/pipeline duplication.
   - Do not introduce a generic ECS, scene file format, asset manager, shader stack, or ephemeris loader until a plan calls for it.

---

## Current technical notes

- Gravity is Newtonian point-mass acceleration.
- Time stepping uses velocity-Verlet / kick-drift-kick.
- The Sun remains fixed at the origin for the current heliocentric baseline.
- The app currently caps physics substeps at five simulation minutes to keep short-period moons stable under frame-scaled time.
- Trails are app-owned dynamic arrays and intentionally keep the full recorded motion history for a run.
- Illustrative render mode enlarges bodies and separates close moons visually without changing simulation data.

---

## Commenting standard

Jonathan wants this codebase to be useful for learning. Add comments where they teach the model or prevent future mistakes:

- explain units and coordinate-frame assumptions;
- explain formulas such as vis-viva and velocity-Verlet;
- explain why a render-only transform exists and why it must not leak into simulation state;
- explain GitHub Actions / WebAssembly pipeline steps when those files are added.

Do **not** comment obvious C syntax such as `++i`, simple assignments, or includes unless there is a non-obvious portability reason.

---

## Current GitHub Pages direction

The Pages site is live and should preserve the cartoon cosmic cockpit direction unless Jonathan asks for another pivot:

- `docs/` Astro static site with saturated purple/blue space surfaces, thick cartoon outlines, playful planet art, and source-backed physics copy.
- WebAssembly build artifacts copied into the Pages output.
- Base-path-safe loader for `https://jonathanperis.github.io/solar-system-simulator/`.
- Docs section that explains the source modules, physics, tests, build pipeline, and future body roadmap.
- CI split between native/test/WASM artifacts and Pages deployment.

---

## Git workflow for this repo

- Keep plans under `.hermes/plans/` and commit them when they guide future implementation.
- Before edits, verify path, remote, branch, and status.
- After completing an iteration, run focused verification, commit, and push `main` unless Jonathan requests a PR workflow.
- Never preserve secrets; replace any encountered secret value with `[REDACTED]`.
