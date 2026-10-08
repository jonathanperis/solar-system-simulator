# Solar System Simulator — AGENTS Guide

A physics-first 3D solar system simulator written in C11 with raylib. This repository is intentionally small, educational, and expansion-oriented: every new body should improve the physical model, tests, and documentation without turning the project into a graphics-first engine.

---

## Repository identity

- **Language:** C11 only for simulator/runtime code.
- **Graphics/windowing:** raylib.
- **Architecture:** deterministic SI-unit simulation isolated from rendering.
- **Current scenes:** a 32-body main scene of large bodies (Sun, eight planets, Vesta, Pluto, the Earth and Mars systems and the 18 major moons including Charon) and six system scenes holding every catalogued moon of Jupiter (115), Saturn (291), Uranus (29), Neptune (16) and Pluto (5), plus the Didymos–Dimorphos binary asteroid; 472 bodies in all. The separate small-body atlas contains 1,564,244 pinned records in compressed shards, not in the active scene or HTML body list.
- **Primary goal:** teach and verify orbital mechanics foundations before visual polish.
- **Current public-site direction:** a simulator-first redesign is planned (DESIGN.md, SPEC A99–A105, tasks T84–T85): the live simulation becomes the homepage instrument and Learn, Catalog and About lean notebook pages. Until those tasks land, the archival-atlas site remains live.

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
├── DESIGN.md              # simulator-first instrument + notebook visual direction for the Pages site
├── Makefile               # native build/test entrypoint
├── README.md              # user-facing project status and physics notes
├── src/
│   ├── main.c             # raylib app loop, camera controls, overlays, simulation stepping
│   ├── headless.c         # raylib-free solar-lab CSV/comparison runner
│   ├── lab_web.c          # C-only comparison WebAssembly entrypoint
│   ├── app/               # app-owned helpers testable without opening a window
│   │   ├── body_trails.*  # synchronized motion history; only fixed stars omit trails
│   │   ├── comparison.*  # matched A/B checkpoints and bounded telemetry
│   │   ├── csv_export.*  # shared native/browser/headless SI export
│   │   ├── lab_config.*  # bounded versioned lesson descriptor
│   │   ├── orbit_camera.* # stable orbit camera and family framing math
│   │   ├── simulation_session.* # playback, selection, reset, physical inspector
│   │   └── simulation_step.* # fixed-step clock accumulator
│   ├── render/            # raylib presentation boundary, lighting shaders, textures and render-scale/style policy
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

- Gravity is Newtonian point-mass acceleration. `physics_compute_accelerations` processes targets in structure-of-arrays blocks so compilers vectorize it (WebAssembly builds with `-msimd128`); keep it bit-identical to the scalar formula: same per-target source order, no FMA (`-ffp-contract=off`) and no fast-math.
- Scenes start from the real sky of JD 2461200.5 (2026-06-09): planet barycenters from `planet_epoch.json`, moons and small bodies from the pinned Horizons snapshot `data/scene_epoch.json` (`tools/scene_epoch.py`). Lessons keep the legacy `*_at_perihelion` states. Scene planets with moons carry J2 (Body `j2`, `j2_radius_m`, `pole`); `physics_oblateness_acceleration` is the one J2 formula, used by the kernel, the force breakdown and the potential energy.
- Scenes: the main scene (`solar_system_create_current`, preset `core`) holds the 32 large bodies; `solar_system_create_family` builds the Jupiter/Saturn/Uranus/Neptune/Pluto/Didymos family scenes (presets `*-system`: Sun, Mercury–Neptune at 1–8, Pluto or Didymos at 9, then the complete catalog with major moons first). Moons outside a binary companion (Charon) start around the pair's barycenter with its mass. Keep C (`solar-lab --catalog [SCENE]`) and TypeScript (`mainSceneBodies`, `familySceneBodies` in `docs/src/lib/bodies.ts`) in the same order; `tools/check_catalog.mjs` checks all seven scenes (main plus six family scenes). A body's `scene` field drives deep links: small moons load their family scene first.
- Unknown-mass moons are explicit test particles. Unknown radius is never a physical zero readout: show Unknown and draw a render-only wire marker. Preserve measured/estimated/unknown provenance.
- Giant-planet satellite mean elements use ecliptic, Laplace or (Uranus's major moons) spin-pole equatorial frames; convert the reference node/pole correctly before adding the planet's absolute state. Legacy initial states remain planar; source-backed Jovian inclinations/retrograde directions are intentional.
- Time stepping uses velocity-Verlet / kick-drift-kick.
- Guided lessons may compare explicit Euler and configure a fixed timestep; core/catalog scenes retain 15-second Verlet. Lesson changes reset the clock and energy baseline. Keep numerical/model/render error distinctions in docs.
- `make headless` builds the raylib-free `solar-lab` CSV runner. `src/app/csv_export.*` serves native, browser and headless exports; never export illustrative coordinates as physical state. Headless `--output` replaces a regular file atomically via a sibling temporary and refuses symlinks/FIFOs/devices; native `E` uses exclusive creation of numbered snapshots. `src/app/input_file.*` is the one bounded reader for experiment/descriptor files. `SOLAR_LAB_MAX_TICKS` (10⁹) caps ticks per comparison side or headless run.
- `src/sim/diagnostics.*` measures massive-body energy/momentum. Normalize energy change by initial kinetic plus absolute potential energy; massless particles contribute no totals and fixed-Sun momentum is not conserved.
- `/learn/compare/` and `solar-lab --compare` share `src/app/comparison.*` and the bounded `SOLAR_LAB_V1` descriptor. Publish only matched checkpoints; partial integration stays private. A merged-away subject is unavailable, never replaced by another body. Charts are presentation of C measurements, not a second physics engine.
- The barycentric-core lesson releases the Sun and preserves relative initial states. Resonance lessons report the resonant angle without claiming that a period ratio proves resonance. Contact response is restricted to the chosen head-on two-sphere lesson, with 0.01–0.25-second steps and explicit bounce/merge policies.
- Automated browser checks use pinned project Playwright with sandbox/TLS validation enabled; `PLAYWRIGHT_CHANNEL=chrome npm run test:browser --prefix docs` uses the built local site. Test fixtures are opt-in and loopback-only.
- The Sun remains fixed at the origin for the current heliocentric baseline.
- Core/catalog playback accumulates frame-scaled time and advances in fixed 15-second physics steps. Accuracy is checked over 100 days against analytical phase and half-step reference solutions.
- Requested speeds extend to 10 and 15 days/second. Each update is capped at 2048 steps, with pending time retained and achieved speed reported. Do not hide slow hardware by dropping time or enlarging steps. The one exception is a stalled frame: `simulation_frame_is_stall()` discards any single frame delta above `SOLAR_APP_STALL_FRAME_SECONDS` (1 s), or a negative/non-finite one, like a background resume; frames at or under 1 s are always kept.
- `make test` checks generated satellite data offline. Only explicit `python3 tools/satellite_catalog.py --refresh [--system NAME]` fetches new source data for the Jovian, Saturnian, Uranian, Neptunian and Plutonian snapshots (JPL satellite tables) and the Didymos snapshot (Horizons DART reconstruction). Review inventory/frame/epoch changes before accepting them; duplicate JPL rows need an explicit ephemeris preference (Uranus's Puck keeps URA184).
- Trails are bounded app-owned histories. They start at a 300-second sample cadence, double both historical and future spacing at compaction, and keep a live endpoint. Do not reintroduce repeated early-history erosion or claim unlimited trail resolution.
- Moving stars must record synchronized history too: parent-relative trails in the barycentric lesson subtract the Sun's historical position. Only fixed stars omit history.
- Illustrative render mode enlarges bodies and separates close moons visually without changing simulation data.
- Saturn's rings are renderer-only textured geometry in Saturn's IAU equatorial plane. Their outer extent affects camera framing, but Saturn's SI radius and gravity remain unchanged.
- The cinematic renderer (SPEC A64–A68) is presentation only: one lighting shader pair (GLSL 330 native / GLSL 100 WebGL) in `src/render/render_resources.c`, raylib-free style math with tests in `src/render/scene_style.*` (texture inventory, atmospheres, IAU WGCCRE 2015 spin orientation, meshes, grid/trail/glow fades), and attributed CC BY 4.0 maps in `assets/textures/` decoded by the vendored, pinned `src/render/third_party/stb_image.h`. Never let lighting, textures or spin feed SI state, CSV or the inspector. Change rendering in screenshot-verified steps (the June beauty pass was rolled back, B5) and measure frame rate against the live site before shipping; per-vertex work in WebAssembly is expensive (avoid `pow` in hot loops), every mesh draw costs dozens of WebGL calls, and trails dominate CPU time: keep `RenderFrameCache` (per-frame parents/positions), screen-adaptive trail detail (`render_trail_stride_for_extent`) and the staggered per-trail extent cache in `draw_trails` (re-measured every eighth frame; it took the 300-body Saturn scene from 37 to 58 fps on a throttled phone). Profile with a `--profiling-funcs` web build and Chrome's CPU profiler under 4× throttling. Translucent layers (trails, clouds, rings, halos, glow) draw after all opaque bodies without depth writes. Labels are screen-space, decluttered by priority and hidden when a nearer large body covers them; the label font is an OFL subset embedded by `tools/embed_binary.py`.
- `src/sim/orbit.c` owns all conic propagation, including standalone WASM previews. Catalog experiments use epoch-aligned Sun/eight planets plus at most 16 selected objects; reset retains their owned names and initial states. Never mix these source-epoch experiments with the perihelion demonstration.
- Preserve double precision until camera-relative subtraction. Grid and trail drawing remain bounded for distant objects.
- `python3 tools/small_body_catalog.py --check` audits every pinned catalog shard offline. Full source refresh is explicit and serialized; keep the source cache in task-owned `build/`, not Git. Browser requests use the pinned Pages assets, not JPL APIs.
- `src/app/simulation_session.*` owns playback, selection, reset, and physical inspection. Paused wall time is excluded; manual steps are one configured tick; reset preserves observer settings. Lesson trail cadence rounds the initial 300 seconds up to whole configured ticks.
- Web controls call the C command boundary in `src/main.c`. Keep numeric command IDs aligned with `runtimeCommands` in `docs/src/lib/simulator.ts` (`tools/check_catalog.mjs` fails on any mismatch); the C scene populates the body selector.
- Parent-relative trail and vector controls are renderer-only. Use synchronized parent samples and subtract in double precision. Vector glyph lengths are illustrative, even when their directions come from SI state.
- Build, CLI, sanitizer and validator entrypoints are `make test-build test-cli test-sanitize test-validators`. On macOS, `make test-native-shaders` renders Earth offscreen through the real GLSL 330 shaders and maps (the CI macOS job runs it with `make test`); use it after shader edits because a native window cannot always be opened from an agent sandbox. Cross-check the C/TypeScript core catalog with `node tools/check_catalog.mjs` after `make headless`.
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

The Pages site is live. DESIGN.md (decided 2026-10-07) is the target for the planned redesign (T84–T85): the simulator becomes the homepage, and Learn, Catalog and About the only other pages. New work follows it; keep copy lean and avoid the generic AI-site patterns it lists.

- `docs/` Astro static site: dark instrument at `/`, warm-paper notebook pages for `/learn/`, `/catalog/` and `/about/`.
- WebAssembly build artifacts copied into the Pages output.
- Astro owns the runtime document and loader integration; Emscripten emits JS/WASM only. The old HTML URL is an Astro-prerendered redirect.
- Base-path-safe loader for `https://jonathanperis.github.io/solar-system-simulator/`.
- One About page explains the model, data sources, build and tests; old documentation URLs redirect to it.
- CI split between native/test/WASM artifacts and Pages deployment.

---

## Git workflow for this repo

- `SPEC.md` owns current goals, constraints, interfaces, invariants, tasks, and bug history. Update it through spec-driven workflow; do not create replacement plan files.
- Before edits, verify path, remote, branch, and status.
- After completing an iteration, run focused verification, commit on a branch, push it and open a PR. `main` is protected by a ruleset: changes land only through a PR with the required Build/CodeQL checks green, resolved review threads and a rebase merge (linear history, no direct pushes).
- Never preserve secrets; replace any encountered secret value with `[REDACTED]`.
