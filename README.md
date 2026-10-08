# solar-system-simulator

A hands-on orbital mechanics and engineering laboratory written in C11 with [raylib](https://www.raylib.com/).

**[Run the simulator](https://jonathanperis.github.io/solar-system-simulator/)** · **[Lessons](https://jonathanperis.github.io/solar-system-simulator/learn/)** · **[Catalog](https://jonathanperis.github.io/solar-system-simulator/catalog/)** · **[About](https://jonathanperis.github.io/solar-system-simulator/about/)**

## Start here

- **Explore:** a 32-body main scene — the Sun, all eight planets, Vesta, Pluto, Earth's Moon, Phobos, Deimos and the 18 major moons of the giant planets and Pluto — plus system scenes with every catalogued moon of Jupiter (115), Saturn (291), Uranus (29), Neptune (16) and Pluto (5), the Didymos–Dimorphos binary asteroid from NASA's DART mission, and the Patroclus–Menoetius binary Jupiter trojan that NASA's Lucy mission will visit. The separate [small-body atlas](https://jonathanperis.github.io/solar-system-simulator/catalog/small-bodies/) contains 1,564,244 pinned records; it does not load them all into the physics scene.
- **Learn:** use [guided lessons](https://jonathanperis.github.io/solar-system-simulator/learn/) and matched A/B comparisons, then export SI measurements as CSV.
- **Run locally:** start with the [raylib-free CLI](#learning-laboratory), or check [prerequisites](#build-prerequisites) before `make && make run` for the 3D app.
- **Contribute:** read [the code map](https://jonathanperis.github.io/solar-system-simulator/about/#code), [build and tests](https://jonathanperis.github.io/solar-system-simulator/about/#build), and the [project layout](#project-layout). [Data provenance](data/README.md) distinguishes pinned measurements, estimates, and unknowns.

## Goal

This project is intentionally physics-first. The renderer exists to show the simulation, but the core work is mathematical: deterministic celestial-body state, SI-unit physics, and testable orbital mechanics foundations.

## Learning laboratory

Start with [guided lessons](https://jonathanperis.github.io/solar-system-simulator/learn/): predict, configure, run, measure, compare, explain. The same C core runs with graphics, in WebAssembly, and through a raylib-free CLI.

```sh
make headless
build/solar-lab --scene circular --days 30 --dt 300 --sample 3600 --output build/verlet.csv
build/solar-lab --scene circular --days 30 --dt 300 --sample 3600 --integrator euler --output build/euler.csv
make test-core test-build test-cli test-validators
```

Lessons include circular, eccentric (`a=1 AU`, `e=0.5`), escape threshold, isolated barycentric Earth–Moon, 30° inclined orbit, and Phobos resolution. Their intentionally artificial initial conditions are separate from the main and planet-system scenes and source-epoch catalog experiments. Lessons allow an initial-speed factor up to 2 and a fixed timestep from 0.01 to 3600 seconds, with the tighter contact bound described below. Each lesson's lowest factor comes from C: the analytic two-body periapsis must stay outside the parent plus subject radius (Phobos 0.73, eccentric 0.12, Earth–Moon 0.21, encounter 0.29; others 0.1), because point-mass gravity has no surface. Orbital lessons and catalog-experiment runs also test every pair's straight-line drift within each step against the sum of radii (the core, barycentric-core and collision lessons are not monitored, so `contact_sphere_crossed=0` there means "not checked", not "clear"); after the first crossing, CSV rows carry `contact_sphere_crossed=1` and analytical reference/phase errors are withheld. The swept test is conservative: a coarse step can cut across a curved arc that never entered the sphere. Euler is a labeled teaching comparison; core/catalog runs retain 15-second Verlet.

The CLI streams CSV with configuration/revision, SI state, ticks, data quality, energy, momentum and center of mass. Duration and sample spacing must align to whole ticks; the final sample is always emitted. A run may contain at most 10⁹ ticks (`duration / dt`), which bounds work to minutes for lesson scenes and under two hours for the largest scene. `--output` must name a new path or an existing regular file you can write, in a directory you can write: symlinks, FIFOs, devices and read-only files are refused, an interrupted run (Ctrl-C/SIGTERM) removes its temporary, and the file is replaced atomically only after a complete run, so a failed run leaves the previous file (or nothing) instead of partial CSV. `--experiment build/selected.tsv` runs a prepared catalog input; `--catalog [SCENE]` exposes the C manifest of the main scene or a planet-system scene for cross-language checks. See `build/solar-lab --help`.

Both visual runtimes offer SI snapshot export, parent-relative history, optional velocity/acceleration directions and scientific diagnostics. Energy change uses `ΔE / (K₀ + |U₀|)` so near-zero escape energy is well-conditioned. Massless tracers contribute no totals; linear momentum is conserved only in unconstrained systems. Physical vectors keep SI values; drawn arrow lengths and illustrative radius magnification are explicitly presentation-only.

### Comparison school

The [A/B comparison lab](https://jonathanperis.github.io/solar-system-simulator/learn/compare/) runs identical initial conditions through two C integrators at matched checkpoints. Live charts show overlaid ecliptic-plane trajectories seen from north, energy change, distance, speed, analytical phase error where applicable, A/B position discrepancy and the resonant angle. Both the browser and CLI use `src/app/comparison.c`; JavaScript only presents C measurements.

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

Methods are `verlet`/`euler`; contact policies are `none`/`bounce`/`merge`. Sample spacing must contain whole ticks for both runs, duration must contain whole samples, and each run is limited to 10⁹ ticks. The browser retains at most 1,025 uniformly coarsened points plus its endpoint within that budget; CLI output streams every checkpoint. A disappeared merged subject is unavailable, never substituted by the surviving body. Force inspectors show source vectors and percentages of summed magnitudes, not percentages of the net vector.

Four additional presets explore specific model choices:

- `barycentric-core`: releases the Sun and translates all 32 main-scene states into a mass-weighted center-of-mass frame, preserving relative initial states. Moving stars record synchronized history so parent-relative trails use the historical Sun position.
- `resonance`: a massless particle starts at an interior 3:2 period ratio with a circular Jupiter perturber. Inspect `3λ_J − 2λ_particle − ϖ_particle` over long runs; a starting period ratio alone does not establish resonance.
- `encounter`: a test particle passes Earth with controlled initial impact geometry. Compare timestep-dependent deflection and minimum integrated distance.
- `collision`: two chosen classroom spheres (10 kg, 10 m radius) approach head-on. Bounce conserves contact kinetic energy/momentum; merge combines mass/volume and explicitly loses kinetic energy. Its 0.01–0.25 s steps prevent tunneling for the allowed initial speeds; contact timing still has finite-step error. Other presets retain point-mass gravity without contact handling.

The 3D collision preset defaults to real scale and slower 1/5/10/25/50 simulated-seconds-per-second playback. Guided browser challenges include a 100-day Phobos phase budget, escape-energy signs, fixed-Sun momentum constraints and export invariance across display changes.

## Complete small-body atlas and all eight planets

The main scene contains the 32 large bodies through Pluto; the system scenes add every catalogued moon of the giant planets and Pluto, and the Didymos and Patroclus binaries. The separate [small-body atlas](https://jonathanperis.github.io/solar-system-simulator/catalog/small-bodies/) exposes all 1,564,244 qualifying entries in the pinned JPL snapshot, including main-belt asteroids, near-Earth asteroids, Trojans, Centaurs and trans-Neptunian bodies. Select up to 16 objects for a C-owned experiment with the Sun and all eight planets.

### Small-body catalog and experiments

- The 2026-09-14 bulk snapshot accounts for 1,568,320 source rows: 1,564,244 qualifying bodies and 4,076 other comet records outside the selected scope. All qualifying entries have usable source orbits.
- Orbital subsets overlap with the overall total: 1,466,940 belt asteroids, 7,287 trans-Neptunian objects and 1,047 Centaurs. The catalog includes named, provisional and hyperbolic objects, including Oumuamua.
- 203 shard pairs (one compressed index and one compressed data file per shard) plus a source-accounted density overview total approximately 141 MB. Search runs in a worker; each result page contains at most 50 rows. Digits-only queries look up an asteroid number or SPK-ID in the shards whose ID range contains it, and unfiltered browsing loads only the shard holding the page. Name and filter searches state the full index download (about 33 MB compressed) and run only after the visitor confirms; that one scan builds a compact in-memory columnar index (about 60 MB) that serves later searches without downloading again. Object details load in the worker from a small cache of data shards. Downloads are SHA-256 verified, so the atlas needs HTTPS or localhost. Catalog, density-cell, result and active-physics counts stay distinct.
- `src/sim/orbit.c` provides the shared universal-variable conic solver for native/WASM physics and orbital previews. Catalog previews two-body propagate source elements to JD 2461200.5 TDB; their original epochs and quality remain visible.
- Experiments initialize all eight planets from `data/planet_epoch.json`, a Sun-centered Horizons vector snapshot of the planetary-system barycenters at the same epoch. With no moons present, each planet stands for its whole system (barycenter state, DE440 system GM), so no unbalanced lunar wobble enters the run. They start explicitly and reset to the same session-owned initial state. The perihelion demonstration scenes remain independently available.
- Missing mass uses a test particle; missing radius stays Unknown. Bulk SBDB physical values are labeled published with unclassified measurement/estimate quality. `data/small_body_physical.json` adds explicitly sourced dwarf-planet measurements/estimates where the bulk catalog lacks them.
- The Sun stays fixed and bodies are point masses. Source epoch alignment does not make subsequent two-body previews or fixed-Sun experiments ephemeris predictions; close encounters require particular numerical caution.

Native selected experiments use the same parser as the browser. Rows must stay physically plausible for small bodies (mass ≤ 10²³ kg, radius ≤ 2,400 km, perihelion ≤ 1,000 AU; every catalog record, Eris included, fits), so a typo cannot overflow the force sum. A headless run whose state nevertheless becomes non-finite stops with a non-zero exit:

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
- Models a 32-body main scene (the original ten bodies, Saturn, Uranus, Neptune, the dwarf planet Pluto and the 18 major moons: Io, Europa, Ganymede, Callisto; Mimas, Enceladus, Tethys, Dione, Rhea, Titan, Iapetus; Ariel, Umbriel, Titania, Oberon, Miranda; Triton; Charon) and seven system scenes (`jupiter-system` 124 bodies, `saturn-system` 300, `uranus-system` 38, `neptune-system` 25, `pluto-system` 15, `didymos-system` 11, `patroclus-system` 11: the Sun, eight planets, the primary and its complete moon catalog). Load a system from the simulator's scene picker, native `K`, or `solar-lab --scene NAME`; links to a small moon open its system. Known radii render as spheres; unknown radii use explicitly nonphysical wire markers.
- Keeps the Sun fixed at the origin for a stable heliocentric baseline.
- Uses right-handed simulation axes: `(x, y, z) = (X, Z, -Y)` of the J2000 ecliptic, so `+y` is ecliptic north and the ecliptic plane is the x/z plane. Prograde orbits have angular momentum along `+y` and appear counterclockwise when viewed from above (north).
- Starts every scene from the real sky of **2026-06-09** (JD 2461200.5 TDB): the planetary-system barycenters from `data/planet_epoch.json` and every moon, Vesta, Pluto, Didymos, Dimorphos, Patroclus and Menoetius from a pinned JPL Horizons vector snapshot (`data/scene_epoch.json`, `tools/scene_epoch.py`). Orbits therefore carry their real tilts. Each family barycenter takes its system's state and the planet sits opposite its moons about it. Two moons Horizons does not serve under their own names (Daphnis, S/2025 U 1) keep mean-element states.
- Shows the scene's calendar date (TDB) next to the elapsed simulated time, and a two-body orbital period for the selected body.
- Gives Earth, Mars, Jupiter, Saturn, Uranus and Neptune their J2 oblateness (NASA fact sheets, IAU poles): each pulls its own moons with the J2 term and feels the reaction. Guided lessons keep point masses so their analytic references hold. Lessons still start from the legacy planar perihelion states.
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
- Core/catalog playback uses a fixed 15-second simulation step and carries frame remainders in an accumulator. Requested presets are one hour, one day (default), five days, ten days, and fifteen days per real second. Each update executes at most 2048 steps, retaining unconsumed time and reporting achieved speed/pending time. Display-frame partitioning does not change the sequence of physics steps once pending work is drained. Hidden-tab/minimized-window wall time is excluded, including the first resumed frame. A single frame longer than one second (laptop sleep with the window visible, a debugger pause) is discarded the same way; shorter slow frames keep their pending time.
- `tests/test_simulation_step.c` verifies less than one degree of isolated Phobos/Deimos phase error over 100 days and less than 1% parent-relative position discrepancy against half-sized steps for the main scene, and over 20 days for each planet-system scene (including Saturn's co-orbital and trojan moons). These are numerical accuracy checks, not ephemeris validation.
- The default core/catalog Sun stays fixed. The explicit barycentric-core lesson releases it and starts in the center-of-mass frame.
- This is a deterministic physics baseline, not an ephemeris. It starts from the real sky of 2026-06-09 and then follows its own model: a fixed Sun, point masses plus planetary J2, no relativity, no tides and no bodies outside the scene, so it drifts away from the real sky over time.

Current simulation data:

Baseline planet values follow NASA/JPL references. Sun, Mercury, Venus, Earth, Moon and Mars masses are `GM / G` from DE440 GM values ([JPL astrodynamic parameters](https://ssd.jpl.nasa.gov/astro_par.html), [planetary physical parameters](https://ssd.jpl.nasa.gov/planets/phys_par.html)) with CODATA 2018 `G = 6.67430e-11`; Jupiter through Neptune use [JPL physical parameters](https://ssd.jpl.nasa.gov/planets/phys_par.html) and [JPL approximate orbital elements](https://ssd.jpl.nasa.gov/planets/approx_pos.html). Satellite values follow JPL Solar System Dynamics. Vesta's pinned physical values and osculating elements are attributed to JPL SBDB solution 36; the [live SBDB query](https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=4%20Vesta&phys-par=1&full-prec=1) may return a newer solution. Derived periapsis distances and vis-viva speeds are calculated in `src/sim/constants.h`. See the [provenance and precision policy](data/README.md#provenance-and-precision-policy) for missing legacy retrieval dates and uncertainty limitations.

| Body | Mass | Radius | Initial state (2026-06-09) |
|---|---:|---:|---|
| Sun | `1.98841e30 kg` | `695700000 m` | fixed at origin |
| Mercury | `3.301001e23 kg` | `2439700 m` | Horizons heliocentric state |
| Venus | `4.867306e24 kg` | `6051800 m` | Horizons heliocentric state |
| Earth | `5.972168e24 kg` | `6371000 m` | Earth–Moon barycenter at its Horizons state; Earth opposite the Moon about it |
| Moon | `7.345789e22 kg` | `1737400 m` | Horizons geocentric state |
| Mars | `6.416909e23 kg` | `3389500 m` | Mars-system barycenter at its Horizons state |
| Phobos, Deimos | `1.06e16 kg`, `1.44e15 kg` | `11080 m`, `6200 m` | Horizons areocentric states |
| Vesta | `2.590276793071933e20 kg` | `261385 m` | Horizons heliocentric state |
| Jupiter–Neptune | planet-only GM / G | JPL mean radii | system barycenters at their Horizons states |
| Pluto, Didymos, Patroclus | Horizons GM / G | JPL radii | system barycenters at their Horizons states |

### Giant-planet satellite data and scenes

Versioned catalogs for [Jupiter](data/jovian_moons.json) (115),
[Saturn](data/saturnian_moons.json) (291), [Uranus](data/uranian_moons.json) (29)
and [Neptune](data/neptunian_moons.json) (16) supply both generated C
initialization and Astro metadata. See [data provenance](data/README.md) for
source links, units, epochs, and frame conversion.

The main scene carries the large bodies only. Its giant-planet moons are the 17
major moons with a measured GM of at least 2 km³/s²: the Galilean moons, Saturn's
seven rounded moons, Uranus's five and Triton. Every other moon runs in its
planet's system scene, which holds the Sun, the eight planets and that planet's
complete catalog (major moons first). Keeping hundreds of small moons out of the
main scene and a vectorized gravity kernel make it about five times faster than
the former 128-body scene (172 versus 32 simulated days per wall second
natively) and less cluttered.
Native throughput by scene: Jupiter system 58, Saturn system 20 (its 300 bodies
fall short of the 15 days/s preset on slower devices, where the achieved rate is
reported), Uranus and Neptune systems above 150 days/second. Every scene holds
about 60 fps on desktop Chrome and 58–59 fps on a 4×-CPU-throttled phone
emulation at the default speed.

Three small-body systems follow the same rules. Pluto (Horizons GM, SBDB orbit)
and Charon belong to the main scene: Charon is 12% of Pluto's mass, so Pluto
circles a barycenter about 2,100 km outside its own surface, and the pair turns
retrograde seen from the ecliptic. Nix, Hydra, Kerberos and Styx circle the
Pluto–Charon pair, so they start around its barycenter with its total mass; their
two-body periods are still 1.5–3.7% longer than JPL's, because JPL's mean orbits
include the binary's rotating field. The `didymos-system` scene holds the DART
target: Dimorphos on its retrograde 11.8-hour orbit about Didymos, from the
Horizons s547 post-impact reconstruction, with estimated masses. The
`patroclus-system` scene holds the binary Jupiter trojan Patroclus and Menoetius
from the Horizons asteroid-satellite solution JPL#82: Menoetius carries 22% of
the pair's mass, so, like Pluto, Patroclus circles a barycenter about 150 km
from its centre, outside its 56.5 km radius, on a retrograde 4.3-day orbit.
Other asteroid moons (Ida's Dactyl, Kalliope's Linus, …) have only literature
orbits in JPL's Small-Body Database, not an ephemeris, and are not modeled.

Moons without a JPL physical-table GM have unknown mass: they feel known-source
gravity with no gravitational backreaction, and moons without a radius draw
wire markers that never claim a physical size. Measured, estimated, and unknown
values remain distinct. Mean orbital elements describe shape/orientation, not a
dated ephemeris or an exact resonant configuration; each moon keeps its ephemeris
solution's epoch, so phases are consistent only within one solution. Point-mass
periods differ from JPL's by at most 0.75% because the model omits oblateness.

Normal builds are offline. `python3 tools/satellite_catalog.py --check` detects
stale generated C data; `--refresh` explicitly updates the source snapshot for
review. `make build/benchmark_simulation && build/benchmark_simulation` measures
headless fixed-step/trail throughput of the main scene, independently of
rendering; `build/benchmark_simulation --scene saturn-system` measures a
planet-system scene.

Mercury orbital values used for initialization:

- semi-major axis: `57909050000 m`
- eccentricity: `0.205630`
- perihelion distance: `semi-major axis * (1 - eccentricity)` = `46001212048.5 m`
- perihelion speed: `58976.392351713628 m/s`, computed from `sqrt(G * SunMass * (2 / perihelion - 1 / semiMajorAxis))`

Venus orbital values used for initialization:

- semi-major axis: `108208000000 m`
- eccentricity: `0.006772`
- perihelion distance: `semi-major axis * (1 - eccentricity)` = `107475215424.0 m`
- perihelion speed: `35258.774979642702 m/s`, computed from `sqrt(G * SunMass * (2 / perihelion - 1 / semiMajorAxis))`

Earth orbital values used for initialization:

- semi-major axis: `149597887155.76578 m`
- eccentricity: `0.01671022`
- perihelion distance: `semi-major axis * (1 - eccentricity)` = `147098073549.85776 m`
- perihelion speed: `30286.627706705909 m/s`, computed from `sqrt(G * SunMass * (2 / perihelion - 1 / semiMajorAxis))`

Moon orbital values used for initialization around Earth:

- semi-major axis: `384400000 m`
- eccentricity: `0.0549`
- perigee distance: `semi-major axis * (1 - eccentricity)` = `363296440 m`
- perigee relative speed: `1082.426923000336 m/s`, computed from `sqrt(G * (EarthMass + MoonMass) * (2 / perigee - 1 / semiMajorAxis))`
- absolute Moon state: Earth heliocentric state plus the Earth-relative perigee offset and relative tangential velocity
- The Earth–Moon barycenter, not Earth itself, takes the heliocentric perihelion state: Earth and the Moon are shifted together by minus their mass-weighted offset (Earth sits about 4,670 km from the barycenter), so the pair does not drift off its intended orbit. The separate Earth–Moon lesson initializes an isolated barycentric pair.

Mars orbital values used for initialization:

- semi-major axis: `227900000000 m`
- eccentricity: `0.0934`
- perihelion distance: `semi-major axis * (1 - eccentricity)` = `206614140000 m`
- aphelion distance: `semi-major axis * (1 + eccentricity)` = `249185860000 m`
- perihelion speed: `26501.187322605019 m/s`, computed from `sqrt(G * SunMass * (2 / perihelion - 1 / semiMajorAxis))`

Martian moon orbital values used for initialization around Mars:

- Phobos semi-major axis: `9377000 m`
- Phobos eccentricity: `0.0151`
- Phobos periareion distance: `semi-major axis * (1 - eccentricity)` = `9235407.3 m`
- Phobos apoareion distance: `semi-major axis * (1 + eccentricity)` = `9518592.7 m`
- Phobos periareion relative speed: `2169.6625368064592 m/s`, computed from `sqrt(G * (MarsMass + PhobosMass) * (2 / periareion - 1 / semiMajorAxis))`
- Deimos semi-major axis: `23460000 m`
- Deimos eccentricity: `0.00033`
- Deimos periareion distance: `semi-major axis * (1 - eccentricity)` = `23452258.2 m`
- Deimos apoareion distance: `semi-major axis * (1 + eccentricity)` = `23467741.8 m`
- Deimos periareion relative speed: `1351.5904459364303 m/s`, computed from `sqrt(G * (MarsMass + DeimosMass) * (2 / periareion - 1 / semiMajorAxis))`
- absolute Phobos/Deimos state: Mars heliocentric state plus each moon's Mars-relative periareion offset and relative tangential velocity; the Mars family is then shifted so its barycenter takes the perihelion state. Jupiter and its known-mass moons follow the same rule; massless test particles carry no weight.

Vesta orbital values used for initialization:

- JPL GM: `17288284400 m^3/s^2`; mass derives from `GM / G`
- effective diameter: `522770 m`; spherical radius: `261385 m`
- semi-major axis: `353255320326.53925 m`
- eccentricity: `0.09020374382834395`
- perihelion distance: `semi-major axis * (1 - eccentricity)` = `321390367905.8045 m`
- perihelion speed: `21217.451749827014 m/s`, computed from `sqrt(G * SunMass * (2 / perihelion - 1 / semiMajorAxis))`
- These values set the planar perihelion state that guided lessons and tests use. The astronomy scenes start Vesta from its Horizons state on 2026-06-09 instead, so there it carries its real 7.1° inclination.

Jupiter orbital values used for initialization:

- mass: `1.8981246e27 kg`; spherical radius from the JPL mean radius: `69911000 m`
- semi-major axis: `778340816692.7108 m`
- eccentricity: `0.04838624`
- perihelion distance: `semi-major axis * (1 - eccentricity)` = `740679831134.4213 m`
- perihelion speed: `13705.69975716819 m/s`, computed from `sqrt(G * SunMass * (2 / perihelion - 1 / semiMajorAxis))`
- These values set the planar perihelion state that guided lessons and tests use. The astronomy scenes start the Jovian-system barycenter from its Horizons state on 2026-06-09 instead, with Jupiter's real 1.3° inclination.

Saturn orbital values used for initialization:

- mass: `5.6831737e26 kg`; spherical radius from the JPL mean radius: `58232000 m`
- semi-major axis: `1426666414179.921 m`
- eccentricity: `0.05386179`
- perihelion distance: `semi-major axis * (1 - eccentricity)` = `1349823607379.3088 m`
- perihelion speed: `10179.094275183943 m/s`, computed from `sqrt(G * SunMass * (2 / perihelion - 1 / semiMajorAxis))`
- Saturn's planar perihelion state serves lessons and tests; the astronomy scenes start from its Horizons system-barycenter state with the real 2.5° inclination. Its visible ring system uses [NASA's roughly `282000 km` overall extent](https://science.nasa.gov/saturn/facts/) and lies in Saturn's equatorial plane from its IAU pole (about 27° from ecliptic north), only at the rendering boundary.

## Rendering model

Rendering code lives under `src/render/` and converts simulation state at the boundary.

- raylib handles windowing, camera, 3D drawing, and overlays.
- Physics units are isolated from rendering units.
- Position scale: `1 AU = 10 render units`.
- Physical radii remain real in simulation data.
- Saturn's textured rings are renderer-only. Their source-backed outer extent affects camera framing, while the physical body radius and all simulation state remain unchanged.
- **Cinematic look (presentation only).** Bodies are lit by the rendered Sun in linear light (Lambert shading, small ambient fill); Earth shows night-side city lights, clouds and an ocean sun glint, bodies with air get a glowing limb, Saturn and its rings shadow each other, and the Sun is emissive with limb darkening and an additive halo over a Milky Way backdrop. Framing a body turns the camera to its sunlit side. Decluttered labels name the Sun, planets, the selected body and visible moons of a zoomed family (JetBrains Mono subset embedded at build time, SIL OFL; see `assets/fonts/README.md`). One shader pair serves desktop (GLSL 330) and WebGL (GLSL 100); 4× multisampling smooths edges.
- **Textures.** The Sun, the eight planets, the Moon, Saturn's rings and the backdrop use [Solar System Scope](https://www.solarsystemscope.com/textures/) maps (CC BY 4.0; see `assets/textures/README.md`). Other moons and Vesta stay lit colours rather than borrowed art. A vendored, pinned `stb_image.h` decodes them because raylib ships with JPEG disabled. The native app reads `assets/textures/` (or `SOLAR_TEXTURE_DIR`); the browser fetches the same files after the first frame. A missing map only means a lit-colour fallback.
- **Spin orientation.** Textured bodies turn about their real axes using the IAU WGCCRE 2015 pole and prime-meridian models, converted from the ICRF through the J2000 ecliptic: Uranus lies on its side, Venus spins backwards. Spin time counts TDB days from J2000 (catalog experiments from their JD 2461200.5 epoch).
- Illustrative mode is the default: planets keep the previous large visible radius, asteroids use a distinct `0.03` render-unit radius, and moons render smaller in proportion to Earth's physical radius with a small visible floor for tiny moons. Parent-relative moon offsets are expanded only in illustrative mode as needed so the large visual spheres remain readable without changing the underlying physics state.
- Trails start with one historical sample per 300 simulated seconds, rounded up to whole configured ticks in lessons. At the 1,025-point budget, historical spacing and future sampling cadence both double. This preserves distributed coverage instead of repeatedly erasing early curvature. The current endpoint updates on every physics step; parent/child sample times stay synchronized, including the moving Sun in the barycentric lesson.
- Resolution decreases uniformly during long runs. Fine satellite loops eventually become less resolved; trails are an approximation of recorded motion, not complete predicted orbital ellipses. The browser reports the current historical spacing.
- The ground grid sits just below the orbital plane and adapts to zoom: power-of-ten minor and major lines (one render unit is 0.1 AU) cross-fade between decades and fade with distance, so they never crowd into moiré. It writes no depth, so bodies always draw over it, and `G` hides it. Trails fade with age, stop at each body's surface and are drawn only as finely as their on-screen size needs. Bodies behind the camera are skipped and sub-pixel bodies drawn as small dots, which keeps the scenes at 60 fps even on a CPU-throttled phone emulation.
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
- `T`: absolute or parent-relative trail history. `X`: vector directions (green velocity, orange acceleration; illustrative lengths). `G`: show or hide the reference grid (web: View → Grid). `H`: show or hide body labels.
- `E`: export an SI snapshot. The browser downloads `solar-snapshot.csv`; native writes the next free `solar-snapshot-001.csv`, `-002.csv`, … in the working directory and never overwrites an earlier snapshot. Headless series and snapshots share the C writer.
- `Space`: pause/resume. Paused time does not accumulate for later catch-up.
- `N`: advance one configured physics tick while paused (15 seconds in core/catalog scenes).
- `R`: restore initial physics, trail history, and clock remainder; retain selection, speed, pause state, render mode, and camera rotation setting.
- `[` / `]`: change requested orbital playback speed among 1 hour, 1 day, 5 days, 10 days, and 15 days per real second without changing the integration step. The collision lesson uses 1/5/10/25/50 simulated seconds per real second instead.
- `1`–`9`, `0`: select the first ten positions in the active scene; in the core demonstration these are the original ten bodies, with `0` selecting Jupiter. `C` cycles the complete active scene.
- `B`: frame only the selected body or its unknown-radius marker.
- `/` in the native app: search by name, provisional designation, or moon group. Down finds the next match, Enter selects, and Escape closes search. Web controls provide a search field and group filter.
- `A`: toggle camera auto-rotation independently of playback.
- `F`: frame the selected planet and its moons. A selected moon frames its parent and siblings; the Sun frames all implemented bodies. Framing fits the current rendered bounding sphere to the viewport and turns the camera to the framed body's sunlit side (about 40° off the Sun line), so a newly selected planet is seen by day. Reframe after motion or manual zoom when needed.
- `V`: toggle visualization mode.
  - Illustrative: physical planetary positions with large visible planet radii, smaller moon radii, and expanded parent-moon visual separation.
  - Real scale: physical orbital positions and physical radii under the same render scale; planets may be nearly invisible.
- `Tab` or `C`: cycle camera focus across all active bodies in the native app.
- `C`: cycle camera focus in the web app; `Tab` remains available for browser navigation.
- Mouse wheel: zoom camera in/out around the current camera target.
  - Zoom distance is clamped.
  - The viewing pitch remains fixed so max zoom-in does not flip or corrupt the camera orientation.

The browser offers labelled buttons, speed/body selectors, and a camera-rotation checkbox. Shortcuts require canvas focus; Tab and form keys remain browser-native. Direct selection restores the default camera distance; framing adjusts zoom bounds/sensitivity and follows the family root. Resizing or changing scale refits an already framed system.

The live physics inspector displays parent-relative distance (km), parent-relative speed (km/s), the two-body orbital period about the parent, mass (kg), and physical radius (km) with their data quality, calculated from C-owned SI state. The Sun's parent-relative measurements are N/A. Physical values are independent of illustrative radii and moon spacing. The sky clock above the view shows the scene's calendar date (TDB) to the minute, or a lesson's elapsed time, with the days since the start.

## Build prerequisites

- C compiler with C11 support
- `make`
- `pkg-config`
- [raylib](https://www.raylib.com/) development libraries for the app build

Use raylib **6.0** for the visual app (including `rlSetClipPlanes`). The headless runner and `make test-core` do not need raylib or a window. Python **3.10+** runs the offline catalog/build checks. Web builds use Emscripten **6.0.9** and a raylib archive compiled with the same SDK. Docs require Node **24+** (CI uses 26), npm and the committed lockfile.

The docs use Astro **7.3.6**, `@astrojs/check` **0.9.10**, and TypeScript **6.0.3**. TypeScript 7 is outside the checker's supported peer range; upgrade it when that tooling supports it. Emscripten 6 targets Chrome 85+, Firefox 79+, and Safari 14.1+; the app also requires WebGL and the catalog uses modern browser APIs, so use a current browser rather than treating those compiler minimums as a tested support matrix.

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

Additional verification: `make test-build test-cli test-validators`, `make test-sanitize`, `make test-native-shaders` (macOS: renders Earth offscreen with the real GLSL 330 shaders and maps; CI runs it on a macOS runner), and `node tools/check_catalog.mjs` after `make headless`. The catalog check compares C and TypeScript names, kinds, parents and order. Native and web exporters use physical coordinates; changing view scale never changes a CSV observation.

Automated browser coverage uses pinned `@playwright/test` 1.63.0 against the built site:

```sh
PLAYWRIGHT_CHANNEL=chrome npm run test:browser --prefix docs
```

Local tests use installed Chrome in isolated contexts; CI installs pinned Chromium. The browser sandbox and TLS validation stay enabled. `tools/serve_site.py` serves only the built tree on loopback; opt-in local failure fixtures exercise missing/invalid assets without network mocking. CI checks the C comparison module against native output before browser tests and Pages packaging.

## Browser runtime

The [live simulator](https://jonathanperis.github.io/solar-system-simulator/) is the site's home page: an Astro page whose canvas fills the window under one dock, an inspector and small Find/View/Keys/Data sheets. Emscripten compiles the same C source into a JavaScript loader and `.wasm` binary; Astro owns the canvas, accessible readouts and loading errors. Explanations live on Learn, Catalog and About. Retired addresses (`/simulator/`, `/compare/`, `/docs/…`, `/wasm/solar-system-simulator.html` and others) are Astro redirect pages that keep `?body=` and fragments.

Every published page is an Astro page, including that compatibility redirect (rendered from `docs/src/pages/wasm/solar-system-simulator.html.astro` and published at its historic filename) and the generated `sitemap.xml`. `make docs-check` fails if a generated HTML file lacks the layout's Astro generator marker or if `docs/public/` contains HTML, a sitemap, or a robots file; scriptable SVG/XHTML/SHTML files need an explicit allow-list, and the noindex `404.html` comes from `docs/src/pages/404.astro`. Each page also carries a strict Content-Security-Policy (same-origin scripts, styles, fonts and connections plus `'wasm-unsafe-eval'`; Google Analytics hosts only on deployed `main`), and fonts are self-hosted under the SIL Open Font License.

Link previews use `docs/public/social-preview.png` (1200×630: a simulator frame of Jupiter and the Galilean moons beside the title), and the icons come from `docs/public/favicon.svg`: an amber Sun inside a tilted orbit on the instrument's dark background. `node tools/site_images.mjs <served base URL>` regenerates the card, `favicon.ico`, the 32 px PNG and the Apple touch icon from the built site into `docs/public/`; rebuild the site (`npm run build --prefix docs`) before `make docs-check` or a preview, which read `docs/dist/`. The capture is deterministic: rotation off, four simulated days after the 2026-06-09 sky. `make docs-check` requires the card's size, the link-preview tags on every page and a script-free SVG.

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

Upgrading an older checkout? Move any legacy generated `docs/public/wasm/solar-system-simulator.html` outside `docs/public/` before building. `make docs-check` rejects any HTML file under `docs/public/`.

For content-only edits, reuse a validated runtime bundle, then rerun the docs tests, check, build, and `make docs-check`. Rebuild runtime assets after C changes. For live content editing, use `npm run dev:background --prefix docs`; inspect or stop that server with `dev:status`, `dev:logs`, and `dev:stop`. The static site and documentation live together in `docs/`.

Local/fork builds omit analytics. CI gives build and validation the same analytics setting. The Build workflow checks native tests, sanitizers, WASM, catalogs, dependencies, Astro output, and browser interactions. Deploy Pages publishes that exact checked tree after a successful same-repository `main` push or manual Build; stale revisions are skipped. CodeQL separately analyzes C/C++, TypeScript/JavaScript, and Actions. No server-side runtime is needed by the published site.

## Project layout

```text
src/
├── app/                # sessions, stepping, trails, camera, CSV, comparison and descriptors
├── headless.c          # raylib-free solar-lab CLI
├── lab_web.c           # C-only comparison WebAssembly entrypoint
├── main.c             # raylib app loop, camera, overlay, simulation stepping
├── render/            # raylib drawing, lighting shaders, textures, render-scale and raylib-free style policy
assets/textures/       # attributed planet/Sun/backdrop maps (CC BY 4.0), copied into the site by make docs-textures
└── sim/               # raylib-independent physics/data model

docs/src/pages/        # static Astro site: simulator home, Learn, Catalog, About, redirects
docs/src/lib/          # presentation, C bridges, catalog worker and shared site metadata
docs/public/catalog/   # pinned compressed small-body snapshot
docs/tests/            # Node tests; docs/browser-tests/ holds browser checks
data/                  # source provenance, giant-planet moon catalogs and planetary epoch snapshot
examples/              # replayable SOLAR_LAB_V1 comparison descriptors
tools/                 # source importers, artifact/route validators and CI helpers
tests/                 # C tests and Python CLI/build/validator tests
.github/workflows/     # Build, checked-artifact Pages deployment and CodeQL
SPEC.md                # current contracts, roadmap, acceptance and audit history
PRODUCT.md / DESIGN.md # learning goals and the instrument/notebook visual direction
```

When updating documentation, check shared claims in the README, `docs/src/lib/site.ts`, `docs/src/lib/sourceMap.ts`, and the relevant Learn, Catalog or About page. The sitemap is generated from the Astro page modules; `make docs-check` verifies the published routes, assets, sitemap, Astro ownership, and CSP. Catalog counts describe pinned snapshots, not automatically refreshed live inventories.

## Next planned iterations

Each future body or moon system is added one iteration at a time, with physical constants, initial conditions, tests, and rendering checks scoped to it. Every system with a JPL ephemeris now has a scene (SPEC A78–A91, A106): all giant-planet moons, Pluto's five moons and the Didymos and Patroclus binaries. Other asteroid moons (Ida's Dactyl, Kalliope's Linus, …) have only literature orbits, no JPL ephemeris this project can pin (SPEC R23); they wait for one.
