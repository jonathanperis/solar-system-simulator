# solar-system-simulator

A bare-bones 3D solar system simulator written in C with [raylib](https://www.raylib.com/).

## Goal

This project is intentionally physics-first. The renderer exists to show the simulation, but the core work is mathematical: deterministic celestial-body state, SI-unit physics, and testable orbital mechanics foundations.

## Milestone 9: Foundation + Sun + Mercury + Venus + Earth + Moon + Mars + Phobos + Deimos + Vesta + Jupiter

The current milestone adds Jupiter as the first gas giant. Its Galilean moons, additional planets, asteroid populations, dwarf planets, textures, shaders, and visual polish remain deferred to later iterations.

Current milestone behavior:

- Opens a raylib 3D scene titled `Solar System Simulator`.
- Renders exactly ten celestial bodies: the Sun, Mercury, Venus, Earth, Moon, Mars, Phobos, Deimos, Vesta, and Jupiter.
- Keeps the Sun fixed at the origin for a stable heliocentric baseline.
- Initializes Mercury at perihelion on the +X axis with tangential +Z velocity from the vis-viva equation.
- Initializes Venus at perihelion on the -X axis with tangential -Z velocity from the vis-viva equation.
- Initializes Earth at perihelion on the +Z axis with tangential -X velocity from the vis-viva equation.
- Initializes the Moon at Earth-relative perigee with tangential relative velocity from the Earth-Moon vis-viva equation.
- Initializes Mars at heliocentric perihelion on the -Z axis with tangential +X velocity from the vis-viva equation.
- Initializes Phobos and Deimos at Mars-relative periareion with tangential relative velocities from the Mars-moon vis-viva equations.
- Initializes Vesta at heliocentric perihelion on the +X axis with tangential +Z velocity from the vis-viva equation.
- Initializes Jupiter at heliocentric perihelion on the -X axis with tangential -Z velocity from the vis-viva equation.
- Advances Mercury, Venus, Earth, the Moon, Mars, Phobos, Deimos, Vesta, and Jupiter with Newtonian gravity from all simulated bodies using the shared simulation integrator.
- Supports illustrative/default and real-scale visualization modes.
- Draws bounded motion traces for every non-star body, with uniform full-run sampling that coarsens as the run grows and an always-current endpoint.
- Allows camera focus cycling across every simulated body: Sun, Mercury, Venus, Earth, Moon, Mars, Phobos, Deimos, Vesta, and Jupiter.
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
- Time stepping uses a velocity-Verlet / kick-drift-kick integrator.
- The app uses a fixed 15-second simulation step and carries frame remainders in an accumulator. The default speed advances one simulated day per real second; presets also provide one hour or five days per second. Display-frame partitioning does not change the sequence of physics steps.
- `tests/test_simulation_step.c` verifies less than one degree of isolated Phobos/Deimos phase error over 100 days and less than 1% parent-relative position discrepancy against half-sized steps for the full ten-body scene. These are numerical accuracy checks, not ephemeris validation.
- The Sun is fixed for this milestone; barycentric Sun motion is deferred.
- This is a deterministic physics baseline, not an ephemeris-accurate model. It does not include relativistic precession, dated J2000 state vectors, measured orbital inclinations, barycentric Earth-Moon initialization, or perturbations from bodies beyond the modeled ten-body scene.

Current simulation data:

Baseline planet values follow NASA/JPL references; Jupiter's values use [JPL physical parameters](https://ssd.jpl.nasa.gov/planets/phys_par.html) and [JPL approximate orbital elements](https://ssd.jpl.nasa.gov/planets/approx_pos.html). Satellite values follow JPL Solar System Dynamics. Vesta's physical values and osculating elements use [JPL SBDB solution 36](https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=4%20Vesta&phys-par=1&full-prec=1). Derived periapsis distances and vis-viva speeds are calculated in `src/sim/constants.h`.

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
- Earth-Moon barycentric initialization is deferred; Earth keeps its existing heliocentric perihelion state for this milestone

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

## Rendering model

Rendering code lives under `src/render/` and converts simulation state at the boundary.

- raylib handles windowing, camera, 3D drawing, and overlays.
- Physics units are isolated from rendering units.
- Position scale: `1 AU = 10 render units`.
- Physical radii remain real in simulation data.
- Illustrative mode is the default: planets keep the previous large visible radius, asteroids use a distinct `0.03` render-unit radius, and moons render smaller in proportion to Earth's physical radius with a small visible floor for tiny moons. Parent-relative moon offsets are expanded only in illustrative mode as needed so the large visual spheres remain readable without changing the underlying physics state.
- Trails start with one historical sample per 300 simulated seconds. At the 1,025-point budget, historical spacing and future sampling cadence both double. This preserves distributed coverage instead of repeatedly erasing early curvature. The current endpoint updates on every physics step; parent/child sample times stay synchronized.
- Resolution decreases uniformly during long runs. Fine satellite loops eventually become less resolved; trails are an approximation of recorded motion, not complete predicted orbital ellipses. The browser reports the current historical spacing.
- The subdued ground grid sits below the orbital plane and expands from the farthest rendered body. Its one-render-unit cells represent 0.1 AU. Solid body colors are not obscured by universal wireframe overlays.
- Real-scale mode uses the same physical render scale for both positions and radii with no radius clamp. Planets may be nearly invisible in this mode; that is physically expected at solar-system scale.

## Camera model

The app uses a small stable orbit camera instead of raylib's automatic orbital helper.

- Camera target follows the selected body, or the family root while system framing is active.
- When enabled, camera auto-rotation orbits around the current camera target independently of physics playback.
- Mouse-wheel input changes only camera distance.
- Zoom distance is clamped between a minimum and maximum value.
- Pitch remains fixed at the default viewing angle, so zooming all the way in and then back out does not flip or corrupt the camera orientation.

## Controls

- `Space`: pause/resume. Paused time does not accumulate for later catch-up.
- `N`: advance exactly 15 simulated seconds while paused.
- `R`: restore initial physics, trail history, and clock remainder; retain selection, speed, pause state, render mode, and camera rotation setting.
- `[` / `]`: change speed among 1 hour, 1 day, and 5 days per real second without changing the integration step.
- `1`–`9`, `0`: select a body directly by catalog position; `0` selects tenth body Jupiter.
- `A`: toggle camera auto-rotation independently of playback.
- `F`: frame the selected planet and its moons. A selected moon frames its parent and siblings; the Sun frames all implemented bodies. Framing fits the current rendered bounding sphere to the viewport. Reframe after motion or manual zoom when needed.
- `V`: toggle visualization mode.
  - Illustrative: physical planetary positions with large visible planet radii, smaller moon radii, and expanded parent-moon visual separation.
  - Real scale: physical orbital positions and physical radii under the same render scale; planets may be nearly invisible.
- `Tab` or `C`: cycle camera focus across Sun, Mercury, Venus, Earth, Moon, Mars, Phobos, Deimos, Vesta, and Jupiter in the native app.
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

On systems where `pkg-config --libs raylib` is unavailable, the Makefile falls back to:

1. `$(HOME)/.local/include` and `$(HOME)/.local/lib` if a local raylib install exists.
2. A conventional Linux raylib link line.

## Commands

```bash
make       # build the raylib app at build/solar-system-simulator
make test  # run C test binaries for simulation math/physics and camera math
make run   # launch the simulator
make clean # remove build outputs
```

## Browser runtime

The [live simulator](https://jonathanperis.github.io/solar-system-simulator/simulator/) is an Astro page using the shared site layout. Emscripten compiles the same C source into a JavaScript loader and `.wasm` binary; Astro owns the canvas, accessible readouts, loading errors, and explanatory content. The previous `/wasm/solar-system-simulator.html` address redirects to `/simulator/`.

```sh
make web RAYLIB_WEB_SRC=/path/to/raylib/src
make dist-wasm RAYLIB_WEB_SRC=/path/to/raylib/src
mkdir -p docs/public/wasm
cp build/web/solar-system-simulator.js build/web/solar-system-simulator.wasm docs/public/wasm/
npm ci --prefix docs
npm test --prefix docs
npm run check --prefix docs
npm run build --prefix docs
make docs-check
```

`make dist-wasm` packages the two runtime assets, not a standalone HTML app. The Build workflow checks native tests, WASM, and Astro output; Deploy Pages publishes the matching successful revision. No server-side runtime is needed by the published site.

## Project layout

```text
src/
├── app/                # small app-level helpers that are testable without opening a window
├── main.c             # raylib app loop, camera, overlay, simulation stepping
├── render/            # raylib drawing code
└── sim/               # raylib-independent physics/data model

tests/                 # C test binaries for simulation and app math
```

## Next planned iterations

Each future body should be added one iteration at a time, with physical constants, initial conditions, tests, and rendering checks scoped to that body.

1. Galilean moons
2. Saturn
3. major Saturnian moons
4. Uranus
5. Neptune
6. dwarf planets / Kuiper belt representatives
