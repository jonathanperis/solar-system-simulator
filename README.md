# solar-system-simulator

A bare-bones 3D solar system simulator written in C with [raylib](https://www.raylib.com/).

## Goal

This project is intentionally physics-first. The renderer exists to show the simulation, but the core work is mathematical: deterministic celestial-body state, SI-unit physics, and testable orbital mechanics foundations.

## Milestone 3: Foundation + Sun + Mercury + Venus

The current milestone extends the foundation to two orbiting planets: Mercury and Venus. Additional planets, moons, asteroids, dwarf planets, textures, shaders, and visual polish are intentionally deferred to later iterations.

Current milestone behavior:

- Opens a raylib 3D scene titled `Solar System Simulator`.
- Renders exactly three celestial bodies: the Sun, Mercury, and Venus.
- Keeps the Sun fixed at the origin for a stable heliocentric baseline.
- Initializes Mercury at perihelion on the +X axis with tangential +Z velocity from the vis-viva equation.
- Initializes Venus at perihelion on the -X axis with tangential -Z velocity from the vis-viva equation.
- Advances Mercury and Venus with Newtonian gravity from all simulated bodies using the shared simulation integrator.
- Supports illustrative/default and real-scale visualization modes.
- Allows camera focus cycling across every simulated body: Sun, Mercury, and Venus.
- Displays body count, elapsed simulation days, time scale, view mode, camera focus target, and render scale notes.

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
- This is a deterministic physics baseline, not an ephemeris-accurate model. It does not include relativistic precession, J2000 state vectors, or perturbations from bodies beyond the Sun, Mercury, and Venus.

Current simulation data:

| Body | Mass | Radius | Initial state |
|---|---:|---:|---|
| Sun | `1.98847e30 kg` | `695700000 m` | fixed at origin |
| Mercury | `3.3011e23 kg` | `2439700 m` | perihelion position and tangential speed |
| Venus | `4.8675e24 kg` | `6051800 m` | perihelion position and tangential speed |

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

## Rendering model

Rendering code lives under `src/render/` and converts simulation state at the boundary.

- raylib handles windowing, camera, 3D drawing, and overlays.
- Physics units are isolated from rendering units.
- Position scale: `1 AU = 10 render units`.
- Physical radii remain real in simulation data.
- Illustrative mode is the default: orbital positions use the physical render scale, while body radii are clamped per body class for visibility.
- Real-scale mode uses the same physical render scale for both positions and radii with no radius clamp. Planets may be nearly invisible in this mode; that is physically expected at solar-system scale.

## Controls

- `V`: toggle visualization mode.
  - Illustrative: physical orbital positions with clamped/enlarged visible body radii.
  - Real scale: physical orbital positions and physical radii under the same render scale; planets may be nearly invisible.
- `Tab` or `C`: cycle camera focus across Sun, Mercury, and Venus.

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
make test  # run C test binaries for simulation math/physics
make run   # launch the simulator
make clean # remove build outputs
```

## Project layout

```text
src/
├── main.c              # raylib app loop, camera, overlay, simulation stepping
├── render/             # raylib drawing code
└── sim/                # raylib-independent physics/data model

tests/                  # C test binaries for simulation code
```

## Next planned iterations

Each future body should be added one iteration at a time, with physical constants, initial conditions, tests, and rendering checks scoped to that body.

1. Earth
2. Moon
3. Mars
4. asteroid belt representatives / major asteroids
5. Jupiter
6. Galilean moons
7. Saturn
8. major Saturnian moons
9. Uranus
10. Neptune
11. dwarf planets / Kuiper belt representatives
