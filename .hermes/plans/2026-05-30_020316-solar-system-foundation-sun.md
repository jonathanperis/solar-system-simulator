# Solar System Foundation + Sun Implementation Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task. Keep this plan local unless Jonathan explicitly asks to commit it.

**Goal:** Build the first bare-bones 3D solar-system simulator foundation in C with raylib, rendering only the Sun while establishing physics-first data structures, SI-unit simulation math, and validation tests.

**Architecture:** Keep simulation physics independent from rendering. Store all physical state in double-precision SI units, expose deterministic step functions that can be tested without opening a raylib window, and convert to raylib `Vector3` only at the render boundary. The first visible scene contains the Sun fixed near the barycentric origin, a simple camera, overlays for simulation time/scale, and no speculative planet-specific code beyond generic body support.

**Tech Stack:** C99/C11, raylib, raymath, Makefile, standard C test binaries using `assert`/return codes.

---

## Progress Snapshot

| Field | Status |
|---|---|
| Repo | `/opt/data/github/jonathanperis/solar-system-simulator` |
| Remote | `origin https://github.com/jonathanperis/solar-system-simulator.git` |
| Branch/ref inspected | `main` at `f82316e` |
| Working tree before plan | Clean (`git status --porcelain` returned no changes) |
| Existing project shape | README-only seed repo plus C-oriented `.gitignore`; no source/build/test layout yet |
| External docs checked | raylib website/repo, raylib cheatsheet, raymath cheatsheet |
| Implementation status | Not started; planning only |
| Validation status | Not applicable yet |

Overall planning progress: foundation and Sun milestone is specified. Later planet/moon iterations are intentionally deferred but will reuse the body/physics APIs created here.

---

## Requirements and Non-Goals

### Requirements

- Use raylib for windowing and 3D drawing.
- Use C only for application/source/test code.
- Prioritize physics/math correctness over visual polish.
- Use SI units internally:
  - meters (`m`)
  - kilograms (`kg`)
  - seconds (`s`)
  - meters/second (`m/s`)
  - Newtonian gravitational constant (`G`)
- Build a generic simulation foundation before adding planets.
- First milestone renders only the Sun.
- Tests must validate math/physics functions without a graphics window.

### Non-Goals for This Milestone

- No planet/moon implementation yet.
- No textures, shaders, skyboxes, orbital trails, UI framework, or asset pipeline.
- No relativistic gravity, radiation pressure, tides, oblateness/J2 perturbations, or N-body optimization.
- No cross-platform packaging beyond a simple local Makefile.
- No Python/JS tooling for implementation or tests.

---

## Physics Design Decisions

1. **Physics uses custom double vectors, not raylib `Vector3`.**
   - raylib `Vector3` stores `float`, which is fine for rendering but too lossy for astronomical state.
   - Use `Vec3d` for simulation and convert to `Vector3` only for drawing.

2. **Use Newtonian point-mass gravity as the baseline.**
   - Acceleration on body `i`: `a_i = Σ G * m_j * (r_j - r_i) / |r_j - r_i|^3`.
   - The Sun-only milestone should keep acceleration zero because there are no other bodies.

3. **Use a symplectic-friendly integrator.**
   - Foundation should implement velocity Verlet/leapfrog, even though Sun-only state does not move.
   - This prevents the next planet iterations from starting with an unstable explicit Euler integrator.

4. **Separate physical radius from rendered radius.**
   - The Sun's true radius is much smaller than astronomical-distance display scales.
   - Store the real radius; use a render policy that clamps visual radius so it is visible.

5. **Start with a single inertial frame.**
   - Origin at Solar System barycentric/Sun position for milestone 1.
   - Later planet iterations can initialize heliocentric state vectors and eventually barycentric corrections.

---

## Proposed File Layout

```text
.
├── Makefile
├── README.md
├── src/
│   ├── main.c
│   ├── render/
│   │   ├── renderer.c
│   │   └── renderer.h
│   └── sim/
│       ├── body.c
│       ├── body.h
│       ├── constants.h
│       ├── physics.c
│       ├── physics.h
│       ├── solar_system.c
│       ├── solar_system.h
│       ├── units.c
│       ├── units.h
│       ├── vec3d.c
│       └── vec3d.h
└── tests/
    ├── test_physics.c
    ├── test_solar_system.c
    └── test_vec3d.c
```

---

## Build Model

Use a Makefile with separate app and test targets.

Expected developer commands:

```bash
make
make run
make test
make clean
```

Initial raylib linkage should be conventional Linux/pkg-config first:

```make
RAYLIB_CFLAGS ?= $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LIBS ?= $(shell pkg-config --libs raylib 2>/dev/null || echo -lraylib -lm -ldl -lpthread -lGL -lrt -lX11)
```

If local CI/container lacks raylib, implementation should document installation rather than vendoring raylib immediately. Vendoring or submodules are a later decision.

---

## Task Plan

### Task 1: Create C project skeleton and Makefile

**Objective:** Establish build/test/run commands without application logic.

**Files:**
- Create: `Makefile`
- Create directories: `src/sim/`, `src/render/`, `tests/`
- Modify: `README.md`

**Steps:**

1. Create the directory structure.
2. Add a Makefile with:
   - `CC ?= cc`
   - `CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -g`
   - `APP := build/solar-system-simulator`
   - `TEST_DIR := build/tests`
   - `all`, `run`, `test`, `clean` targets
   - app source list separate from test source lists
3. Add README sections:
   - project goal
   - physics-first design
   - build prerequisites (`raylib`, `pkg-config`, C compiler, `make`)
   - commands (`make`, `make run`, `make test`)
4. Run `make test`.

**Expected verification:**

- `make test` succeeds once placeholder test targets exist, or reports no tests only for this task.
- `git status --short` shows only intended skeleton files.

**Commit suggestion:** `chore: add C raylib project skeleton`

---

### Task 2: Add double-precision vector math foundation

**Objective:** Provide simulation-safe vector operations independent from raylib.

**Files:**
- Create: `src/sim/vec3d.h`
- Create: `src/sim/vec3d.c`
- Create: `tests/test_vec3d.c`
- Modify: `Makefile`

**Core API:**

```c
#ifndef SOLAR_VEC3D_H
#define SOLAR_VEC3D_H

typedef struct Vec3d {
    double x;
    double y;
    double z;
} Vec3d;

Vec3d vec3d_zero(void);
Vec3d vec3d_add(Vec3d a, Vec3d b);
Vec3d vec3d_sub(Vec3d a, Vec3d b);
Vec3d vec3d_scale(Vec3d v, double scalar);
double vec3d_dot(Vec3d a, Vec3d b);
double vec3d_length_squared(Vec3d v);
double vec3d_length(Vec3d v);

#endif
```

**Test cases:**

- Addition/subtraction preserve components.
- Scaling works for positive/negative scalars.
- Dot product of orthogonal vectors is zero.
- Length of `(3, 4, 12)` is `13` within epsilon.

**Verification:**

```bash
make test
```

Expected: `test_vec3d` passes.

**Commit suggestion:** `feat: add double precision vector math`

---

### Task 3: Add physical constants and unit conversion helpers

**Objective:** Centralize SI constants and render scaling policy.

**Files:**
- Create: `src/sim/constants.h`
- Create: `src/sim/units.h`
- Create: `src/sim/units.c`
- Modify: `tests/test_vec3d.c` or create a small unit test section if desired
- Modify: `Makefile`

**Constants:**

```c
#ifndef SOLAR_CONSTANTS_H
#define SOLAR_CONSTANTS_H

#define SOLAR_G 6.67430e-11
#define SOLAR_AU_METERS 149597870700.0
#define SOLAR_DAY_SECONDS 86400.0

#define SOLAR_SUN_MASS_KG 1.98847e30
#define SOLAR_SUN_RADIUS_M 695700000.0

#endif
```

**Unit API:**

```c
#ifndef SOLAR_UNITS_H
#define SOLAR_UNITS_H

#include "vec3d.h"

float meters_to_render_units(double meters);
Vec3d meters_vec_to_render_vec3d(Vec3d meters);

double seconds_to_days(double seconds);

#endif
```

**Policy:**

- `1 AU = 10.0 render units` for positions.
- Keep this in one place so visual scale can change later without touching physics.

**Verification:**

- Test `meters_to_render_units(SOLAR_AU_METERS) == 10.0f` within float epsilon.
- Test `seconds_to_days(SOLAR_DAY_SECONDS) == 1.0`.

**Commit suggestion:** `feat: add astronomical constants and unit scaling`

---

### Task 4: Define generic celestial body model

**Objective:** Represent the Sun and future planets/moons with one reusable body type.

**Files:**
- Create: `src/sim/body.h`
- Create: `src/sim/body.c`
- Create or modify: `tests/test_solar_system.c`
- Modify: `Makefile`

**Core API:**

```c
#ifndef SOLAR_BODY_H
#define SOLAR_BODY_H

#include <stdbool.h>
#include "vec3d.h"

typedef enum BodyKind {
    BODY_KIND_STAR,
    BODY_KIND_PLANET,
    BODY_KIND_MOON,
    BODY_KIND_ASTEROID,
    BODY_KIND_DWARF_PLANET
} BodyKind;

typedef struct Body {
    const char *name;
    BodyKind kind;
    double mass_kg;
    double radius_m;
    Vec3d position_m;
    Vec3d velocity_mps;
    Vec3d acceleration_mps2;
    bool fixed;
} Body;

Body body_create(
    const char *name,
    BodyKind kind,
    double mass_kg,
    double radius_m,
    Vec3d position_m,
    Vec3d velocity_mps,
    bool fixed
);

#endif
```

**Design notes:**

- `fixed` is allowed for the first Sun milestone so the Sun remains visually stable.
- Later, when multiple bodies are added, `fixed` can be turned off for barycentric motion or retained as a simulation option.

**Verification:**

- Test body creation preserves all fields.
- Test Sun body can be created at zero position/velocity with known mass/radius.

**Commit suggestion:** `feat: add generic celestial body model`

---

### Task 5: Add Newtonian acceleration calculation

**Objective:** Compute pairwise gravitational acceleration in SI units.

**Files:**
- Create: `src/sim/physics.h`
- Create: `src/sim/physics.c`
- Create: `tests/test_physics.c`
- Modify: `Makefile`

**Core API:**

```c
#ifndef SOLAR_PHYSICS_H
#define SOLAR_PHYSICS_H

#include <stddef.h>
#include "body.h"
#include "vec3d.h"

Vec3d gravitational_acceleration_from(const Body *target, const Body *source);
void physics_compute_accelerations(Body *bodies, size_t body_count);

#endif
```

**Numerical behavior:**

- If `target == source`, contribution is zero.
- If distance squared is zero, contribution is zero to avoid division by zero.
- Otherwise use `G * source->mass_kg / r^3 * displacement`.

**Test cases:**

- Sun-only body acceleration is zero.
- A 1 kg target at 1 AU from Sun has acceleration magnitude close to `G * M_sun / AU^2`.
- Direction points toward the source mass.

**Verification:**

```bash
make test
```

Expected: vector, units, body, and physics tests pass.

**Commit suggestion:** `feat: compute Newtonian gravitational acceleration`

---

### Task 6: Add deterministic simulation stepping

**Objective:** Advance bodies with a stable integrator suitable for orbital simulations.

**Files:**
- Modify: `src/sim/physics.h`
- Modify: `src/sim/physics.c`
- Modify: `tests/test_physics.c`

**Core API addition:**

```c
void physics_step(Body *bodies, size_t body_count, double dt_seconds);
```

**Integrator:**

Use velocity Verlet / kick-drift-kick:

1. Compute current accelerations.
2. Half-kick velocity by `0.5 * a * dt`.
3. Drift position by `v * dt`.
4. Recompute accelerations.
5. Half-kick velocity again.
6. Skip position/velocity updates for `fixed` bodies but still allow them to contribute gravity.

**Test cases:**

- Sun-only fixed body remains at origin after one day.
- Non-fixed body with no acceleration moves linearly if velocity is non-zero and body count is one.
- Fixed body does not drift even with non-zero velocity.

**Verification:**

```bash
make test
```

Expected: all tests pass deterministically.

**Commit suggestion:** `feat: add deterministic physics stepping`

---

### Task 7: Create initial solar-system state with only the Sun

**Objective:** Encapsulate milestone-1 initial conditions and simulation state.

**Files:**
- Create: `src/sim/solar_system.h`
- Create: `src/sim/solar_system.c`
- Modify: `tests/test_solar_system.c`
- Modify: `Makefile`

**Core API:**

```c
#ifndef SOLAR_SYSTEM_H
#define SOLAR_SYSTEM_H

#include <stddef.h>
#include "body.h"

typedef struct SolarSystem {
    Body bodies[1];
    size_t body_count;
    double elapsed_seconds;
} SolarSystem;

SolarSystem solar_system_create_sun_only(void);
void solar_system_step(SolarSystem *system, double dt_seconds);

#endif
```

**Test cases:**

- `body_count == 1`.
- First body name is `Sun`.
- First body kind is `BODY_KIND_STAR`.
- Sun mass/radius match constants.
- Step increments `elapsed_seconds` and keeps Sun fixed at origin.

**Future-proofing without overengineering:**

- Use `bodies[1]` now because only the Sun is in scope.
- The first planet iteration can resize this to a named capacity constant, e.g. `SOLAR_MAX_BODIES`, when it actually needs multiple bodies.

**Commit suggestion:** `feat: initialize sun-only solar system`

---

### Task 8: Add raylib renderer for the Sun

**Objective:** Draw a simple 3D scene containing the Sun from simulation state.

**Files:**
- Create: `src/render/renderer.h`
- Create: `src/render/renderer.c`
- Modify: `Makefile`

**Renderer API:**

```c
#ifndef SOLAR_RENDERER_H
#define SOLAR_RENDERER_H

#include "../sim/solar_system.h"

void renderer_draw_solar_system(const SolarSystem *system);

#endif
```

**Implementation outline:**

- Use raylib 3D mode in `main.c`; renderer only draws bodies.
- Convert `Body.position_m` to render units.
- Draw Sun as a yellow/orange sphere:
  - true radius converted to render units will be tiny relative to distances;
  - clamp visible Sun radius to something like `0.5f` render units for milestone visibility;
  - keep physical radius in data untouched.
- Draw a small origin/grid reference using raylib helpers if useful (`DrawGrid`).

**Raylib functions expected:**

- `BeginMode3D(camera)` / `EndMode3D()`
- `DrawSphere(position, radius, color)`
- `DrawGrid(slices, spacing)`
- `DrawText(...)` for overlay outside 3D mode

**Verification:**

- App compiles and links with raylib.
- Running `make run` opens a window with one visible Sun at origin.

**Commit suggestion:** `feat: render sun in raylib 3d scene`

---

### Task 9: Add application loop and camera controls

**Objective:** Wire simulation stepping and rendering into a minimal interactive app.

**Files:**
- Create: `src/main.c`
- Modify: `Makefile`
- Modify: `README.md`

**Main loop behavior:**

- Window title: `Solar System Simulator`
- Camera:
  - position around `(0, 8, 18)`
  - target origin
  - perspective projection
  - use `UpdateCamera(&camera, CAMERA_ORBITAL)` or a simple fixed camera for first pass
- Simulation:
  - create `SolarSystem system = solar_system_create_sun_only();`
  - choose `time_scale = SOLAR_DAY_SECONDS` per real second as a starting value
  - each frame: `solar_system_step(&system, GetFrameTime() * time_scale);`
- Overlay:
  - body count
  - elapsed days
  - time scale
  - note: `Physics: SI Newtonian baseline; rendering scale: 1 AU = 10 units`

**Verification:**

```bash
make
make test
make run
```

Expected:

- Build succeeds.
- Tests pass.
- Window opens with a single Sun, grid/origin reference, and overlay.
- Sun remains stable at origin over time.

**Commit suggestion:** `feat: add sun-only simulator loop`

---

### Task 10: Document milestone acceptance criteria and next iterations

**Objective:** Make the first milestone explicit and prepare future planet/moon increments.

**Files:**
- Modify: `README.md`

**README content to add:**

- Milestone 1: Foundation + Sun
- Physics model:
  - SI units
  - double precision
  - Newtonian gravity
  - velocity Verlet stepping
- Rendering model:
  - raylib 3D
  - render scale separate from physics units
- Next planned increments:
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

**Verification:**

- README explains how to build, test, and run from a fresh clone.
- README does not promise features not yet implemented.

**Commit suggestion:** `docs: document sun foundation milestone`

---

## Acceptance Criteria for Milestone 1

- `make` builds `build/solar-system-simulator`.
- `make test` runs all C test binaries and passes.
- `make run` launches a raylib window.
- The scene renders exactly one celestial body: the Sun.
- The Sun uses real mass and radius in simulation data.
- Simulation state uses double-precision SI units.
- Rendering scale is isolated from simulation units.
- Physics code can compute Newtonian acceleration generically for future bodies.
- Sun-only stepping is deterministic and leaves the fixed Sun at origin.
- README documents dependencies, commands, and milestone scope.

---

## Risks and Mitigations

| Risk | Mitigation |
|---|---|
| raylib not installed in local/container environment | Keep physics tests independent from raylib; document install; link via `pkg-config` with fallback libs |
| Astronomical distances lose precision in render floats | Keep physics in `double`; scale only at render boundary |
| Visual radius conflicts with physical radius | Store true radius; use explicit render clamp/policy |
| Integrator instability in future planet steps | Start with velocity Verlet/leapfrog, not explicit Euler |
| Scope creep into graphics polish | No textures/shaders/assets until physics foundation and body iterations are solid |
| Future planet initial conditions require authoritative ephemerides | Defer to each planet iteration; document source/date/reference frame per body |

---

## Open Questions for Later Iterations

These do not block the Sun foundation:

1. Should future planet initial states use J2000 orbital elements, NASA Horizons state vectors, or simplified circular approximations for the first pass?
2. Should the simulation eventually use barycentric coordinates with all bodies mobile, or retain a heliocentric fixed-Sun mode as a toggle?
3. Which “major pieces of stone” are in scope after planets/moons: dwarf planets, named asteroids, Kuiper belt objects, comets?
4. How strict should long-term energy/angular-momentum conservation tests be once at least two bodies exist?

---

## Implementation Handoff

Plan complete for the foundation + Sun milestone. Execute with small task-by-task commits, running `make test` after every physics/data task and `make && make test` after every app/build task. Do not add planets, moons, assets, or UI polish until this milestone passes all acceptance criteria.
