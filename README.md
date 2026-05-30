# solar-system-simulator

A bare-bones 3D solar system simulator written in C with [raylib](https://www.raylib.com/).

## Goal

This project is intentionally physics-first. The renderer exists to show the simulation, but the core work is mathematical: deterministic celestial-body state, SI-unit physics, and testable orbital mechanics foundations.

## Milestone 1: Foundation + Sun

The first milestone builds only the simulation foundation and the Sun. Planets, moons, asteroids, dwarf planets, textures, shaders, and visual polish are intentionally deferred to later iterations.

Current milestone behavior:

- Opens a raylib 3D scene titled `Solar System Simulator`.
- Renders exactly one celestial body: the Sun.
- Keeps the Sun fixed at the origin for a stable first visual baseline.
- Displays body count, elapsed simulation days, time scale, and render scale notes.

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
- The Sun uses real simulation data:
  - mass: `1.98847e30 kg`
  - radius: `695700000 m`

## Rendering model

Rendering code lives under `src/render/` and converts simulation state at the boundary.

- raylib handles windowing, camera, 3D drawing, and overlays.
- Physics units are isolated from rendering units.
- Position scale: `1 AU = 10 render units`.
- Physical radii remain real in simulation data, but rendered body radii can be clamped for visibility.

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

1. Mercury
2. Venus
3. Earth
4. Moon
5. Mars
6. asteroid belt representatives / major asteroids
7. Jupiter
8. Galilean moons
9. Saturn
10. major Saturnian moons
11. Uranus
12. Neptune
13. dwarf planets / Kuiper belt representatives
