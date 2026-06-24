# solar-system-simulator

A bare-bones 3D solar system simulator written in C with [raylib](https://www.raylib.com/).

## Goal

This project is intentionally physics-first. The renderer exists to show the simulation, but the core work is mathematical: deterministic celestial-body state, SI-unit physics, and testable orbital mechanics foundations.

## Milestone 7: Foundation + Sun + Mercury + Venus + Earth + Moon + Mars + Phobos + Deimos

The current milestone extends Mars with Phobos and Deimos as Mars-relative natural satellites and adds a first renderer beauty pass at the raylib boundary. Additional planets, asteroids, dwarf planets, textures, shader materials, labels, and ephemeris-grade data are intentionally deferred to later iterations.

Current milestone behavior:

- Opens a raylib 3D scene titled `Solar System Simulator`.
- Renders exactly eight celestial bodies: the Sun, Mercury, Venus, Earth, Moon, Mars, Phobos, and Deimos.
- Keeps the Sun fixed at the origin for a stable heliocentric baseline.
- Initializes Mercury at perihelion on the +X axis with tangential +Z velocity from the vis-viva equation.
- Initializes Venus at perihelion on the -X axis with tangential -Z velocity from the vis-viva equation.
- Initializes Earth at perihelion on the +Z axis with tangential -X velocity from the vis-viva equation.
- Initializes the Moon at Earth-relative perigee with tangential relative velocity from the Earth-Moon vis-viva equation.
- Initializes Mars at heliocentric perihelion on the -Z axis with tangential +X velocity from the vis-viva equation.
- Initializes Phobos and Deimos at Mars-relative periareion with tangential relative velocities from the Mars-moon vis-viva equations.
- Advances Mercury, Venus, Earth, the Moon, Mars, Phobos, and Deimos with Newtonian gravity from all simulated bodies using the shared simulation integrator.
- Supports illustrative/default and real-scale visualization modes.
- Draws persistent fading motion traces for every non-star body so Mercury, Venus, Earth, the Moon, Mars, Phobos, and Deimos leave visible paths as they move.
- Allows camera focus cycling across every simulated body: Sun, Mercury, Venus, Earth, Moon, Mars, Phobos, and Deimos.
- Clamps mouse-wheel camera zoom while preserving the default viewing pitch, so max zoom-in does not flip or corrupt the camera orientation.
- Displays a compact live overlay with elapsed simulation days, focus target, view mode, camera zoom, and controls while static renderer notes live on the website shell.

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
- The Sun is fixed for this milestone; barycentric Sun motion is deferred.
- This is a deterministic physics baseline, not an ephemeris-accurate model. It does not include relativistic precession, J2000 state vectors, barycentric Earth-Moon initialization, or perturbations from bodies beyond the Sun, Mercury, Venus, Earth, Moon, Mars, Phobos, and Deimos.

Current simulation data:

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

## Rendering model

Rendering code lives under `src/render/` and converts simulation state at the boundary.

- raylib handles windowing, camera, 3D drawing, and overlays.
- Physics units are isolated from rendering units.
- Position scale: `1 AU = 10 render units`.
- Physical radii remain real in simulation data.
- Illustrative mode is the default: planets keep the previous large visible radius, while moons render smaller in proportion to Earth's physical radius with a small visible floor for tiny moons. Parent-relative moon offsets are expanded only in illustrative mode as needed so the large visual spheres remain readable without changing the underlying physics state.
- Planet and moon traces keep every simulation position recorded during the run and are drawn before the bodies as age-faded colored line segments, so trails never extinguish while the app is running.
- The ground grid keeps a minimum readable square count and expands from the farthest rendered body, so Mars and later outer planets do not outgrow the visible reference grid.
- The backdrop, starfield, solid Sun styling, body surface accents, lit highlights, and viewport-matched render texture are presentation effects in `src/render/renderer.c` and `src/main.c`; they never feed back into SI-unit body state.
- Real-scale mode uses the same physical render scale for both positions and radii with no radius clamp. Planets may be nearly invisible in this mode; that is physically expected at solar-system scale.

## Camera model

The app uses a small stable orbit camera instead of raylib's automatic orbital helper.

- Camera target eases toward the focused body.
- The camera slowly auto-orbits around the focused body.
- Mouse-wheel input changes only camera distance.
- Zoom distance is clamped between a minimum and maximum value.
- Pitch remains fixed at the default viewing angle, so zooming all the way in and then back out does not flip or corrupt the camera orientation.

## Controls

- `V`: toggle visualization mode.
  - Illustrative: physical planetary positions with large visible planet radii, smaller moon radii, and expanded parent-moon visual separation.
  - Real scale: physical orbital positions and physical radii under the same render scale; planets may be nearly invisible.
- `Tab` or `C`: cycle camera focus across Sun, Mercury, Venus, Earth, Moon, Mars, Phobos, and Deimos.
- Mouse wheel: zoom camera in/out around the focused body.
  - Zoom distance is clamped.
  - The viewing pitch remains fixed so max zoom-in does not flip or corrupt the camera orientation.

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

1. asteroid belt representatives / major asteroids
2. Jupiter
3. Galilean moons
4. Saturn
5. major Saturnian moons
6. Uranus
7. Neptune
8. dwarf planets / Kuiper belt representatives
