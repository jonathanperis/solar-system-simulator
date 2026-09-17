# solar-system-simulator

A hands-on orbital mechanics and engineering laboratory written in C11 with [raylib](https://www.raylib.com/).

**[Run the simulator](https://jonathanperis.github.io/solar-system-simulator/simulator/)** · **[Compare experiments](https://jonathanperis.github.io/solar-system-simulator/compare/)** · **[Read the field guide](https://jonathanperis.github.io/solar-system-simulator/docs/)**

## Start here

- **Explore:** 128 core bodies — the Sun, all eight planets, Vesta, Earth's Moon, Phobos, Deimos, and 115 Jovian moons. The separate [small-body atlas](https://jonathanperis.github.io/solar-system-simulator/small-bodies/) contains 1,564,244 pinned records; it does not load them all into the physics scene.
- **Learn:** use [guided experiments](https://jonathanperis.github.io/solar-system-simulator/docs/experiments/) and matched A/B comparisons, then export SI measurements as CSV.
- **Run locally:** start with the [raylib-free CLI](#learning-laboratory), or check [prerequisites](#build-prerequisites) before `make && make run` for the 3D app.
- **Contribute:** read [architecture](https://jonathanperis.github.io/solar-system-simulator/docs/architecture/), [build and web](https://jonathanperis.github.io/solar-system-simulator/docs/build-and-web/), and the [project layout](#project-layout). [Data provenance](data/README.md) distinguishes pinned measurements, estimates, and unknowns.

## Goal

This project is intentionally physics-first. The renderer exists to show the simulation, but the core work is mathematical: deterministic celestial-body state, SI-unit physics, and testable orbital mechanics foundations.

## Learning laboratory

Start with [guided experiments](https://jonathanperis.github.io/solar-system-simulator/docs/experiments/): predict, configure, run, measure, compare, explain. The same C core runs with graphics, in WebAssembly, and through a raylib-free CLI.

```sh
make headless
build/solar-lab --scene circular --days 30 --dt 300 --sample 3600 --output build/verlet.csv
build/solar-lab --scene circular --days 30 --dt 300 --sample 3600 --integrator euler --output build/euler.csv
make test-core test-build test-cli test-validators
```

Lessons include circular, eccentric (`a=1 AU`, `e=0.5`), escape threshold, isolated barycentric Earth–Moon, 30° inclined orbit, and Phobos resolution. Their intentionally artificial initial conditions are separate from the 128-body core and source-epoch catalog experiments. Lessons allow an initial-speed factor from 0.1 to 2 and a fixed timestep from 0.01 to 3600 seconds, with the tighter contact bound described below. Euler is a labeled teaching comparison; core/catalog runs retain 15-second Verlet.

The CLI streams CSV with configuration/revision, SI state, ticks, data quality, energy, momentum and center of mass. Duration and sample spacing must align to whole ticks; the final sample is always emitted. `--experiment build/selected.tsv` runs a prepared catalog input; `--catalog` exposes the C core manifest for cross-language checks. See `build/solar-lab --help`.

Both visual runtimes offer SI snapshot export, parent-relative history, optional velocity/acceleration directions and scientific diagnostics. Energy change uses `ΔE / (K₀ + |U₀|)` so near-zero escape energy is well-conditioned. Massless tracers contribute no totals; linear momentum is conserved only in unconstrained systems. Physical vectors keep SI values; drawn arrow lengths and illustrative radius magnification are explicitly presentation-only.

### Comparison school

The [A/B comparison lab](https://jonathanperis.github.io/solar-system-simulator/compare/) runs identical initial conditions through two C integrators at matched checkpoints. Live charts show overlaid X/Z trajectories, energy change, distance, speed, analytical phase error where applicable, A/B position discrepancy and the resonant angle. Both the browser and CLI use `src/app/comparison.c`; JavaScript only presents C measurements.

```sh
make headless
build/solar-lab --compare examples/circular.solar > build/comparison.csv
build/solar-lab --compare examples/phobos.solar > build/phobos-comparison.csv
build/solar-lab --compare examples/collision.solar > build/contact-comparison.csv
```

Use **Save configuration**, **Import configuration**, or **Create share link** to reproduce a lesson. Links load parameters without automatically starting a run and show the source revision. The bounded, versioned descriptor is:

```text
SOLAR_LAB_V1 preset factor methodA dtA contactA methodB dtB contactB sample_seconds duration_seconds
```

Methods are `verlet`/`euler`; contact policies are `none`/`bounce`/`merge`. Sample spacing must contain whole ticks for both runs, and duration must contain whole samples. The browser retains at most 1,025 uniformly coarsened points plus its endpoint within that budget; CLI output streams every checkpoint. A disappeared merged subject is unavailable, never substituted by the surviving body. Force inspectors show source vectors and percentages of summed magnitudes, not percentages of the net vector.

Four additional presets explore specific model choices:

- `barycentric-core`: releases the Sun and translates all 128 states into a mass-weighted center-of-mass frame, preserving relative initial states. Moving stars record synchronized history so parent-relative trails use the historical Sun position.
- `resonance`: a massless particle starts at an interior 3:2 period ratio with a circular Jupiter perturber. Inspect `3λ_J − 2λ_particle − ϖ_particle` over long runs; a starting period ratio alone does not establish resonance.
- `encounter`: a test particle passes Earth with controlled initial impact geometry. Compare timestep-dependent deflection and minimum integrated distance.
- `collision`: two chosen classroom spheres (10 kg, 10 m radius) approach head-on. Bounce conserves contact kinetic energy/momentum; merge combines mass/volume and explicitly loses kinetic energy. Its 0.01–0.25 s steps prevent tunneling for the allowed initial speeds; contact timing still has finite-step error. Other presets retain point-mass gravity without contact handling.

The 3D collision preset defaults to real scale and slower 1/5/10/25/50 simulated-seconds-per-second playback. Guided browser challenges include a 100-day Phobos phase budget, escape-energy signs, fixed-Sun momentum constraints and export invariance across display changes.

## Complete small-body atlas and all eight planets

The core demonstration contains 128 bodies through Neptune. The separate [small-body atlas](https://jonathanperis.github.io/solar-system-simulator/small-bodies/) exposes all 1,564,244 qualifying entries in the pinned JPL snapshot, including main-belt asteroids, near-Earth asteroids, Trojans, Centaurs and trans-Neptunian bodies. Select up to 16 objects for a C-owned experiment with the Sun and all eight planets.

### Small-body catalog and experiments

- The 2026-09-14 bulk snapshot accounts for 1,568,320 source rows: 1,564,244 qualifying bodies and 4,076 other comet records outside the selected scope. All qualifying entries have usable source orbits.
- Orbital subsets overlap with the overall total: 1,466,940 belt asteroids, 7,287 trans-Neptunian objects and 1,047 Centaurs. The catalog includes named, provisional and hyperbolic objects, including Oumuamua.
- 203 shard pairs (one compressed index and one compressed data file per shard) plus a source-accounted density overview total approximately 141 MB. Search scans bounded shards in a worker; each result page contains at most 50 rows. Catalog, density-cell, result and active-physics counts stay distinct.
- `src/sim/orbit.c` provides the shared universal-variable conic solver for native/WASM physics and orbital previews. Catalog previews two-body propagate source elements to JD 2461200.5 TDB; their original epochs and quality remain visible.
- Experiments initialize all eight planets from `data/planet_epoch.json`, a Sun-centered Horizons vector snapshot at the same epoch. They start explicitly and reset to the same session-owned initial state. The 128-body perihelion demonstration remains independently available.
- Missing mass uses a test particle; missing radius stays Unknown. Bulk SBDB physical values are labeled published with unclassified measurement/estimate quality. `data/small_body_physical.json` adds explicitly sourced dwarf-planet measurements/estimates where the bulk catalog lacks them.
- The Sun stays fixed and bodies are point masses. Source epoch alignment does not make subsequent two-body previews or fixed-Sun experiments ephemeris predictions; close encounters require particular numerical caution.

Native selected experiments use the same parser as the browser:

```sh
python3 tools/export_experiment.py 20000001 20000004 20134340 --output build/selected.tsv
build/solar-system-simulator --experiment build/selected.tsv
```

Normal verification is offline. Explicit source maintenance uses serialized queries:

```sh
make build/catalog-orbits.dylib
python3 tools/small_body_catalog.py --refresh --generate
python3 tools/small_body_catalog.py --check
python3 tools/planet_epoch.py --refresh
```

Review source counts, quality and generated changes before publishing a refreshed snapshot. A cached bulk response can be regenerated with `--generate` without repeating the JPL request. Browser clients read pinned Pages assets, never the JPL API directly.

Current milestone behavior:

- Opens a raylib 3D scene titled `Solar System Simulator`.
- Models 128 core bodies: the original ten bodies, 115 Jovian moons, Saturn, Uranus and Neptune. Known radii render as spheres; unknown radii use explicitly nonphysical wire markers.
- Keeps the Sun fixed at the origin for a stable heliocentric baseline.
- Initializes Mercury at perihelion on the +X axis with tangential +Z velocity from the vis-viva equation.
- Initializes Venus at perihelion on the -X axis with tangential -Z velocity from the vis-viva equation.
- Initializes Earth at perihelion on the +Z axis with tangential -X velocity from the vis-viva equation.
- Initializes the Moon at Earth-relative perigee with tangential relative velocity from the Earth-Moon vis-viva equation.
- Initializes Mars at heliocentric perihelion on the -Z axis with tangential +X velocity from the vis-viva equation.
- Initializes Phobos and Deimos at Mars-relative periareion with tangential relative velocities from the Mars-moon vis-viva equations.
- Initializes Vesta at heliocentric perihelion on the +X axis with tangential +Z velocity from the vis-viva equation.
- Initializes Jupiter at heliocentric perihelion on the -X axis with tangential -Z velocity from the vis-viva equation.
- Initializes Jovian moons from sourced mean orbital elements, preserving inclination and retrograde/prograde direction through a common-frame conversion, then adds Jupiter's absolute position and velocity.
- Initializes Saturn at heliocentric perihelion on the +Z axis with tangential -X velocity from the vis-viva equation.
- Advances moving bodies with Newtonian gravity from all nonzero-mass sources using the shared simulation integrator. Unknown-mass moons are test particles, not invented physical masses.
- Supports illustrative/default and real-scale visualization modes.
- Draws bounded motion traces for non-star bodies and moving stars, with uniform full-run sampling that coarsens as the run grows and an always-current endpoint. Only fixed stars omit history.
- Allows camera focus cycling and name/group search across all active bodies, with separate family and individual-body framing.
- Clamps mouse-wheel camera zoom while preserving the default viewing pitch, so max zoom-in does not flip or corrupt the camera orientation.
- Displays simulation readouts in the native HUD and accessible Astro page; the browser canvas is reserved for the scene.

## Physics model

Simulation code lives under `src/sim/` and is independent from raylib.

- Internal state uses SI units:
  - meters (`m`)
  - kilograms (`kg`)
  - seconds (`s`)
  - meters per second (`m/s`)
- Simulation vectors use double precision (`Vec3d`) instead of raylib's float `Vector3`.
- Gravity uses the Newtonian point-mass formula:
  - `a = G * source_mass / distance^3 * displacement`
- Core/catalog time stepping uses velocity-Verlet / kick-drift-kick; isolated lessons can compare explicit Euler.
- Core/catalog playback uses a fixed 15-second simulation step and carries frame remainders in an accumulator. Requested presets are one hour, one day (default), five days, ten days, and fifteen days per real second. Each update executes at most 2048 steps, retaining unconsumed time and reporting achieved speed/pending time. Display-frame partitioning does not change the sequence of physics steps once pending work is drained. Hidden-tab/minimized-window wall time is excluded, including the first resumed frame.
- `tests/test_simulation_step.c` verifies less than one degree of isolated Phobos/Deimos phase error over 100 days and less than 1% parent-relative position discrepancy against half-sized steps for the full 128-body scene. These are numerical accuracy checks, not ephemeris validation.
- The default core/catalog Sun stays fixed. The explicit barycentric-core lesson releases it and starts in the center-of-mass frame.
- This is a deterministic physics baseline, not an ephemeris-accurate model. The core perihelion demonstration remains planar through Neptune, with Jovian moon inclinations from their source frames. Catalog experiments align their initial epoch, then use the same fixed-Sun point-mass gravity. Relativity, planetary oblateness and omitted-body perturbations are outside this model.

Current simulation data:

Baseline planet values follow NASA/JPL references; Jupiter through Neptune use [JPL physical parameters](https://ssd.jpl.nasa.gov/planets/phys_par.html) and [JPL approximate orbital elements](https://ssd.jpl.nasa.gov/planets/approx_pos.html). Satellite values follow JPL Solar System Dynamics. Vesta's pinned physical values and osculating elements are attributed to JPL SBDB solution 36; the [live SBDB query](https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=4%20Vesta&phys-par=1&full-prec=1) may return a newer solution. Derived periapsis distances and vis-viva speeds are calculated in `src/sim/constants.h`. See the [provenance and precision policy](data/README.md#provenance-and-precision-policy) for missing legacy retrieval dates and uncertainty limitations.

| Body | Mass | Radius | Initial state |
|---|---:|---:|---|
| Sun | `1.98847e30 kg` | `695700000 m` | fixed at origin |
| Mercury | `3.3011e23 kg` | `2439700 m` | perihelion position and tangential speed |
| Venus | `4.8675e24 kg` | `6051800 m` | perihelion position and tangential speed |
| Earth | `5.9736e24 kg` | `6371000 m` | perihelion position and tangential speed |
| Moon | `7.346e22 kg` | `1737400 m` | Earth-relative perigee offset and tangential relative speed |
| Mars | `6.419e23 kg` | `3390000 m` | perihelion position and tangential speed |
| Phobos | `1.061834199841182e16 kg` | `11080 m` | Mars-relative periareion offset and tangential relative speed |
| Deimos | `1.441349654645431e15 kg` | `6200 m` | Mars-relative periareion offset and tangential relative speed |
| Vesta | `2.590276793071933e20 kg` | `261385 m` | heliocentric perihelion position and tangential speed |
| Jupiter | `1.898125e27 kg` | `69911000 m` | heliocentric perihelion position and tangential speed |
| Saturn | `5.68317e26 kg` | `58232000 m` | heliocentric perihelion position and tangential speed |
| Uranus | `8.68099e25 kg` | `25362000 m` | heliocentric perihelion, `a=19.18916464 AU`, `e=0.04725744` |
| Neptune | `1.024092e26 kg` | `24622000 m` | heliocentric perihelion, `a=30.06992276 AU`, `e=0.00859048` |

### Jovian satellite data and approximations

The complete [versioned catalog](data/jovian_moons.json) supplies both generated
C initialization and Astro metadata. It contains four Galilean moons, four
smaller inner moons, and 107 irregular moons. See [data provenance](data/README.md)
for source links, units, epochs, and frame conversion.

Nine moons have JPL physical-table entries. Published model estimates are
labeled in both inspectors. The other 106 have unknown mass/radius in this
snapshot: they feel known-source gravity with no gravitational backreaction,
and their wire markers in either view never claim a physical size. Measured,
estimated, and unknown values remain distinct. Mean orbital elements describe
shape/orientation, not a dated ephemeris or an exact resonant configuration.

Normal builds are offline. `python3 tools/jovian_catalog.py --check` detects
stale generated C data; `--refresh` explicitly updates the source snapshot for
review. `make build/benchmark_simulation && build/benchmark_simulation` measures
headless full-scene fixed-step/trail throughput, independently of rendering.

Mercury orbital values used for initialization:

- semi-major axis: `57909050000 m`
- eccentricity: `0.205630`
- perihelion distance: `semi-major axis * (1 - eccentricity)` = `46001212048.5 m`
- perihelion speed: `58977.28405570045 m/s`, computed from `sqrt(G * SunMass * (2 / perihelion - 1 / semiMajorAxis))`

Venus orbital values used for initialization:

- semi-major axis: `108208000000 m`
- eccentricity: `0.006772`
- perihelion distance: `semi-major axis * (1 - eccentricity)` = `107475215424.0 m`
- perihelion speed: `35259.30808092215 m/s`, computed from `sqrt(G * SunMass * (2 / perihelion - 1 / semiMajorAxis))`

Earth orbital values used for initialization:

- semi-major axis: `149597887155.76578 m`
- eccentricity: `0.01671022`
- perihelion distance: `semi-major axis * (1 - eccentricity)` = `147098073549.85776 m`
- perihelion speed: `30287.085630725956 m/s`, computed from `sqrt(G * SunMass * (2 / perihelion - 1 / semiMajorAxis))`

Moon orbital values used for initialization around Earth:

- semi-major axis: `384400000 m`
- eccentricity: `0.0549`
- perigee distance: `semi-major axis * (1 - eccentricity)` = `363296440 m`
- perigee relative speed: `1082.5552631364333 m/s`, computed from `sqrt(G * (EarthMass + MoonMass) * (2 / perigee - 1 / semiMajorAxis))`
- absolute Moon state: Earth heliocentric state plus the Earth-relative perigee offset and relative tangential velocity
- The core keeps Earth's existing heliocentric perihelion state. The separate Earth–Moon lesson initializes an isolated barycentric pair.

Mars orbital values used for initialization:

- semi-major axis: `227900000000 m`
- eccentricity: `0.0934`
- perihelion distance: `semi-major axis * (1 - eccentricity)` = `206614140000 m`
- aphelion distance: `semi-major axis * (1 + eccentricity)` = `249185860000 m`
- perihelion speed: `26501.588011990192 m/s`, computed from `sqrt(G * SunMass * (2 / perihelion - 1 / semiMajorAxis))`

Martian moon orbital values used for initialization around Mars:

- Phobos semi-major axis: `9377000 m`
- Phobos eccentricity: `0.0151`
- Phobos periareion distance: `semi-major axis * (1 - eccentricity)` = `9235407.3 m`
- Phobos apoareion distance: `semi-major axis * (1 + eccentricity)` = `9518592.7 m`
- Phobos periareion relative speed: `2170.0160220561597 m/s`, computed from `sqrt(G * (MarsMass + PhobosMass) * (2 / periareion - 1 / semiMajorAxis))`
- Deimos semi-major axis: `23460000 m`
- Deimos eccentricity: `0.00033`
- Deimos periareion distance: `semi-major axis * (1 - eccentricity)` = `23452258.2 m`
- Deimos apoareion distance: `semi-major axis * (1 + eccentricity)` = `23467741.8 m`
- Deimos periareion relative speed: `1351.8106494404324 m/s`, computed from `sqrt(G * (MarsMass + DeimosMass) * (2 / periareion - 1 / semiMajorAxis))`
- absolute Phobos/Deimos state: Mars heliocentric state plus each moon's Mars-relative periareion offset and relative tangential velocity

Vesta orbital values used for initialization:

- JPL GM: `17288284400 m^3/s^2`; mass derives from `GM / G`
- effective diameter: `522770 m`; spherical radius: `261385 m`
- semi-major axis: `353255320326.53925 m`
- eccentricity: `0.09020374382834395`
- perihelion distance: `semi-major axis * (1 - eccentricity)` = `321390367905.8045 m`
- perihelion speed: `21217.7725508384 m/s`, computed from `sqrt(G * SunMass * (2 / perihelion - 1 / semiMajorAxis))`
- Vesta remains in the default X/Z plane. Its measured inclination is deliberately deferred until a dedicated orbital-geometry milestone.

Jupiter orbital values used for initialization:

- mass: `1.898125e27 kg`; spherical radius from the JPL mean radius: `69911000 m`
- semi-major axis: `778340816692.7108 m`
- eccentricity: `0.04838624`
- perihelion distance: `semi-major axis * (1 - eccentricity)` = `740679831134.4213 m`
- perihelion speed: `13705.906982917822 m/s`, computed from `sqrt(G * SunMass * (2 / perihelion - 1 / semiMajorAxis))`
- Jupiter remains in the default X/Z plane. JPL's listed inclination is deliberately deferred until a dedicated orbital-geometry milestone.

Saturn orbital values used for initialization:

- mass: `5.68317e26 kg`; spherical radius from the JPL mean radius: `58232000 m`
- semi-major axis: `1426666414179.921 m`
- eccentricity: `0.05386179`
- perihelion distance: `semi-major axis * (1 - eccentricity)` = `1349823607379.3088 m`
- perihelion speed: `10179.248179798748 m/s`, computed from `sqrt(G * SunMass * (2 / perihelion - 1 / semiMajorAxis))`
- Saturn remains in the default X/Z plane. Its visible ring system uses [NASA's roughly `282000 km` overall extent and `26.73` degree tilt](https://science.nasa.gov/saturn/facts/) only at the rendering boundary.

## Rendering model

Rendering code lives under `src/render/` and converts simulation state at the boundary.

- raylib handles windowing, camera, 3D drawing, and overlays.
- Physics units are isolated from rendering units.
- Position scale: `1 AU = 10 render units`.
- Physical radii remain real in simulation data.
- Saturn's ring lines are renderer-only. Their source-backed outer extent affects camera framing, while the physical body radius and all simulation state remain unchanged.
- Illustrative mode is the default: planets keep the previous large visible radius, asteroids use a distinct `0.03` render-unit radius, and moons render smaller in proportion to Earth's physical radius with a small visible floor for tiny moons. Parent-relative moon offsets are expanded only in illustrative mode as needed so the large visual spheres remain readable without changing the underlying physics state.
- Trails start with one historical sample per 300 simulated seconds, rounded up to whole configured ticks in lessons. At the 1,025-point budget, historical spacing and future sampling cadence both double. This preserves distributed coverage instead of repeatedly erasing early curvature. The current endpoint updates on every physics step; parent/child sample times stay synchronized, including the moving Sun in the barycentric lesson.
- Resolution decreases uniformly during long runs. Fine satellite loops eventually become less resolved; trails are an approximation of recorded motion, not complete predicted orbital ellipses. The browser reports the current historical spacing.
- The subdued ground grid sits below the orbital plane as a bounded camera-local patch of at most 512 slices. Its one-render-unit cells represent 0.1 AU. Solid body colors are not obscured by universal wireframe overlays.
- Real-scale mode uses the same physical render scale for both positions and radii with no radius clamp. Planets may be nearly invisible in this mode; that is physically expected at solar-system scale.

## Camera model

The app uses a small stable orbit camera instead of raylib's automatic orbital helper.

- Camera target follows the selected body, or the family root while system framing is active.
- When enabled, camera auto-rotation orbits around the current camera target independently of physics playback.
- Mouse-wheel input changes only camera distance.
- Zoom distance is clamped between a minimum and maximum value.
- Pitch remains fixed at the default viewing angle, so zooming all the way in and then back out does not flip or corrupt the camera orientation.

## Controls

- Native `L`: cycle lesson presets; `I`: compare integrators; `D`: cycle lesson step size; `-` / `=`: change initial speed. Configuration changes restart the lesson. Browser controls expose the same C-owned configuration explicitly.
- `M`: switch bounce/merge and restart the collision lesson; the browser also provides a Contact button.
- `T`: absolute or parent-relative trail history. `X`: vector directions (green velocity, orange acceleration; illustrative lengths).
- `E`: export an SI snapshot as `solar-snapshot.csv` (browser download or native working directory). Headless series and snapshots share the C writer.
- `Space`: pause/resume. Paused time does not accumulate for later catch-up.
- `N`: advance one configured physics tick while paused (15 seconds in core/catalog scenes).
- `R`: restore initial physics, trail history, and clock remainder; retain selection, speed, pause state, render mode, and camera rotation setting.
- `[` / `]`: change requested orbital playback speed among 1 hour, 1 day, 5 days, 10 days, and 15 days per real second without changing the integration step. The collision lesson uses 1/5/10/25/50 simulated seconds per real second instead.
- `1`–`9`, `0`: select the first ten positions in the active scene; in the core demonstration these are the original ten bodies, with `0` selecting Jupiter. `C` cycles the complete active scene.
- `B`: frame only the selected body or its unknown-radius marker.
- `/` in the native app: search by name, provisional designation, or moon group. Down finds the next match, Enter selects, and Escape closes search. Web controls provide a search field and group filter.
- `A`: toggle camera auto-rotation independently of playback.
- `F`: frame the selected planet and its moons. A selected moon frames its parent and siblings; the Sun frames all implemented bodies. Framing fits the current rendered bounding sphere to the viewport. Reframe after motion or manual zoom when needed.
- `V`: toggle visualization mode.
  - Illustrative: physical planetary positions with large visible planet radii, smaller moon radii, and expanded parent-moon visual separation.
  - Real scale: physical orbital positions and physical radii under the same render scale; planets may be nearly invisible.
- `Tab` or `C`: cycle camera focus across all active bodies in the native app.
- `C`: cycle camera focus in the web app; `Tab` remains available for browser navigation.
- Mouse wheel: zoom camera in/out around the current camera target.
  - Zoom distance is clamped.
  - The viewing pitch remains fixed so max zoom-in does not flip or corrupt the camera orientation.

The browser offers labelled buttons, speed/body selectors, and a camera-rotation checkbox. Shortcuts require canvas focus; Tab and form keys remain browser-native. Direct selection restores the default camera distance; framing adjusts zoom bounds/sensitivity and follows the family root. Resizing or changing scale refits an already framed system.

The live physics inspector displays parent-relative distance (km), parent-relative speed (km/s), mass (kg), and physical radius (km), calculated from C-owned SI state. The Sun's parent-relative measurements are N/A. Physical values are independent of illustrative radii and moon spacing. Elapsed time includes seconds so a single 15-second step remains visible.

## Build prerequisites

- C compiler with C11 support
- `make`
- `pkg-config`
- [raylib](https://www.raylib.com/) development libraries for the app build

Use raylib **6.0** for the visual app (including `rlSetClipPlanes`). The headless runner and `make test-core` do not need raylib or a window. Python **3.10+** runs the offline catalog/build checks. Web builds use Emscripten **6.0.9** and a raylib archive compiled with the same SDK. Docs require Node **24+** (CI uses 26), npm and the committed lockfile.

The docs use Astro **7.3.3**, `@astrojs/check` **0.9.10**, and TypeScript **6.0.3**. TypeScript 7 is outside the checker's supported peer range; upgrade it when that tooling supports it. Emscripten 6 targets Chrome 85+, Firefox 79+, and Safari 14.1+; the app also requires WebGL and the catalog uses modern browser APIs, so use a current browser rather than treating those compiler minimums as a tested support matrix.

For the web library, use a raylib 6.0 source checkout and the same Emscripten SDK as the app. Run `make PLATFORM=PLATFORM_WEB -C /absolute/path/to/raylib/src`; force a platform rebuild when reusing native object files. Follow the complete [browser runtime recipe](#browser-runtime) below to stage every required artifact.

On systems where `pkg-config --libs raylib` is unavailable, the Makefile falls back to:

1. `$(HOME)/.local/include` and `$(HOME)/.local/lib` if a local raylib install exists.
2. A conventional Linux raylib link line.

## Commands

```bash
make       # build the raylib app at build/solar-system-simulator
make test  # run offline catalog checks and simulation/app/renderer C tests
make run   # launch the simulator
make clean # remove build outputs
```

Additional verification: `make test-build test-cli test-validators`, `make test-sanitize`, and `node tools/check_catalog.mjs` after `make headless`. The catalog check compares C and TypeScript names, kinds, parents and order. Native and web exporters use physical coordinates; changing view scale never changes a CSV observation.

Automated browser coverage uses pinned `@playwright/test` 1.63.0 against the built site:

```sh
PLAYWRIGHT_CHANNEL=chrome npm run test:browser --prefix docs
```

Local tests use installed Chrome in isolated contexts; CI installs pinned Chromium. The browser sandbox and TLS validation stay enabled. `tools/serve_site.py` serves only the built tree on loopback; opt-in local failure fixtures exercise missing/invalid assets without network mocking. CI checks the C comparison module against native output before browser tests and Pages packaging.

## Browser runtime

The [live simulator](https://jonathanperis.github.io/solar-system-simulator/simulator/) is an Astro page using the shared site layout. Emscripten compiles the same C source into a JavaScript loader and `.wasm` binary; Astro owns the canvas, accessible readouts, loading errors, and explanatory content. The previous `/wasm/solar-system-simulator.html` address redirects to `/simulator/`.

```sh
make docs-assets RAYLIB_WEB_SRC=/absolute/path/to/raylib/src
npm ci --prefix docs
npm test --prefix docs
npm run check --prefix docs
npm run build --prefix docs
make docs-check
npm run preview --prefix docs
```

Open the printed loopback URL under `/solar-system-simulator/`. Run commands from the repository root. `make docs-assets` validates and stages all five runtime files — `solar-system-simulator.js`, `solar-system-simulator.wasm`, `catalog-orbits.wasm`, `learning-lab.mjs`, and `learning-lab.wasm` — plus `build-info.json`. The manifest records source revision and checksums. `make dist-wasm RAYLIB_WEB_SRC=/absolute/path/to/raylib/src` optionally packages the same six files into a ZIP; Astro supplies the HTML pages.

Upgrading an older checkout? Move any legacy generated `docs/public/wasm/solar-system-simulator.html` outside `docs/public/` before building. A public HTML file at that path shadows Astro's redirect; `make docs-check` rejects the stale standalone page.

For content-only edits, reuse a validated runtime bundle, then rerun the docs tests, check, build, and `make docs-check`. Rebuild runtime assets after C changes. For live content editing, use `npm run dev:background --prefix docs`; inspect or stop that server with `dev:status`, `dev:logs`, and `dev:stop`. The static site and documentation live together in `docs/`.

Local/fork builds omit analytics. CI gives build and validation the same analytics setting. The Build workflow checks native tests, sanitizers, WASM, catalogs, dependencies, Astro output, and browser interactions. Deploy Pages publishes that exact checked tree after a successful same-repository `main` push or manual Build; stale revisions are skipped. CodeQL separately analyzes C/C++, TypeScript/JavaScript, and Actions. No server-side runtime is needed by the published site.

## Project layout

```text
src/
├── app/                # sessions, stepping, trails, camera, CSV, comparison and descriptors
├── headless.c          # raylib-free solar-lab CLI
├── lab_web.c           # C-only comparison WebAssembly entrypoint
├── main.c             # raylib app loop, camera, overlay, simulation stepping
├── render/            # raylib drawing code
└── sim/               # raylib-independent physics/data model

docs/src/pages/        # static Astro site, field guide, simulator and comparison pages
docs/src/lib/          # presentation, C bridges, catalog worker and shared site metadata
docs/public/catalog/   # pinned compressed small-body snapshot
docs/tests/            # Node tests; docs/browser-tests/ holds browser checks
data/                  # source provenance, Jovian catalog and planetary epoch snapshot
examples/              # replayable SOLAR_LAB_V1 comparison descriptors
tools/                 # source importers, artifact/route validators and CI helpers
tests/                 # C tests and Python CLI/build/validator tests
.github/workflows/     # Build, checked-artifact Pages deployment and CodeQL
SPEC.md                # current contracts, roadmap, acceptance and audit history
PRODUCT.md / DESIGN.md # learning goals and archival solar-chart visual direction
```

When updating documentation, check shared claims in the README, `docs/src/lib/site.ts`, `docs/src/lib/sourceMap.ts`, and the relevant field-guide page. Keep new page links and `docs/public/sitemap.xml` aligned; `make docs-check` verifies the published routes, assets, and sitemap. Catalog counts describe pinned snapshots, not automatically refreshed live inventories.

## Next planned iterations

Each future body should be added one iteration at a time, with physical constants, initial conditions, tests, and rendering checks scoped to that body.

1. complete Saturnian moons
2. Uranian and Neptunian moon catalogs
3. Small-body satellite systems
