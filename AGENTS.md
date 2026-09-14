# Solar System Simulator — AGENTS Guide

A physics-first 3D solar system simulator written in C11 with raylib. This repository is intentionally small, educational, and expansion-oriented: every new body should improve the physical model, tests, and documentation without turning the project into a graphics-first engine.

---

## Repository identity

- **Language:** C11 only for simulator/runtime code.
- **Graphics/windowing:** raylib.
- **Architecture:** deterministic SI-unit simulation isolated from rendering.
- **Current scene:** original ten bodies, all 115 Jovian moons, Saturn, Uranus and Neptune; 128 core bodies. The separate small-body atlas contains 1,564,244 pinned records in compressed shards, not in the active scene or HTML body list.
- **Primary goal:** teach and verify orbital mechanics foundations before visual polish.
- **Current public-site direction:** archival solar chart, source-backed and playful, with an accessible illustrative orrery wrapped around SI-unit physics. The docs hub and Astro-owned `/simulator/` runtime share the atlas layout.

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
├── SPEC.md                # current goals, constraints, invariants, tasks, bug history
├── PRODUCT.md             # product and learning context for Impeccable/design work
├── DESIGN.md              # archival solar-chart visual direction for current GitHub Pages work
├── Makefile               # native build/test entrypoint
├── README.md              # user-facing project status and physics notes
├── src/
│   ├── main.c             # raylib app loop, camera controls, overlays, simulation stepping
│   ├── app/               # app-owned helpers testable without opening a window
│   │   ├── body_trails.*  # persistent motion-history storage for non-star bodies
│   │   ├── orbit_camera.* # stable orbit camera and family framing math
│   │   ├── simulation_session.* # playback, selection, reset, physical inspector
│   │   └── simulation_step.* # fixed-step clock accumulator
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
- Unknown-mass moons are explicit test particles. Unknown radius is never a physical zero readout: show Unknown and draw a render-only wire marker. Preserve measured/estimated/unknown provenance.
- Jovian satellite mean elements use ecliptic or Laplace frames; convert the reference node/pole correctly before adding Jupiter's absolute state. Legacy initial states remain planar; source-backed Jovian inclinations/retrograde directions are intentional.
- Time stepping uses velocity-Verlet / kick-drift-kick.
- The Sun remains fixed at the origin for the current heliocentric baseline.
- The app accumulates frame-scaled time and advances in fixed 15-second physics steps. Accuracy is checked over 100 days against analytical phase and half-step reference solutions.
- Requested speeds extend to 10 and 15 days/second. Each update is capped at 2048 steps, with pending time retained and achieved speed reported. Do not hide slow hardware by dropping time or enlarging steps.
- `make test` checks generated satellite data offline. Only explicit `python3 tools/jovian_catalog.py --refresh` fetches new source data. Review inventory/frame/epoch changes before accepting them.
- Trails are bounded app-owned histories. They start at a 300-second sample cadence, double both historical and future spacing at compaction, and keep a live endpoint. Do not reintroduce repeated early-history erosion or claim unlimited trail resolution.
- Illustrative render mode enlarges bodies and separates close moons visually without changing simulation data.
- Saturn's rings are renderer-only lines. Their outer extent affects camera framing, but Saturn's SI radius and gravity remain unchanged.
- `src/sim/orbit.c` owns all conic propagation, including standalone WASM previews. Catalog experiments use epoch-aligned Sun/eight planets plus at most 16 selected objects; reset retains their owned names and initial states. Never mix these source-epoch experiments with the perihelion demonstration.
- Preserve double precision until camera-relative subtraction. Grid and trail drawing remain bounded for distant objects.
- `python3 tools/small_body_catalog.py --check` audits every pinned catalog shard offline. Full source refresh is explicit and serialized; keep the source cache in task-owned `build/`, not Git. Browser requests use the pinned Pages assets, not JPL APIs.
- `src/app/simulation_session.*` owns playback, selection, reset, and physical inspection. Paused wall time is excluded; manual steps are exactly 15 seconds; reset preserves observer settings.
- Web controls call the C command boundary in `src/main.c`. Keep numeric command IDs aligned with `runtimeCommands` in `docs/src/lib/simulator.ts`; the C scene populates the body selector.
- Frame-system uses renderer-only family bounds and aspect-aware camera fitting. Inspector distances/speeds always use parent-relative SI state, independent of render mode.

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

The Pages site is live and should preserve archival solar-chart direction unless Jonathan asks for another pivot:

- `docs/` Astro static site with cream paper, ink-navy chart surfaces, brass markers, accessible atlas interaction, and source-backed physics copy.
- WebAssembly build artifacts copied into the Pages output.
- Astro owns the runtime document and loader integration; Emscripten emits JS/WASM only. The old HTML URL is an Astro-prerendered redirect.
- Base-path-safe loader for `https://jonathanperis.github.io/solar-system-simulator/`.
- Docs section that explains the source modules, physics, tests, build pipeline, and future body roadmap.
- CI split between native/test/WASM artifacts and Pages deployment.

---

## Git workflow for this repo

- `SPEC.md` owns current goals, constraints, interfaces, invariants, tasks, and bug history. Update it through spec-driven workflow; do not create replacement plan files.
- Before edits, verify path, remote, branch, and status.
- After completing an iteration, run focused verification, commit, and push `main` unless Jonathan requests a PR workflow.
- Never preserve secrets; replace any encountered secret value with `[REDACTED]`.
