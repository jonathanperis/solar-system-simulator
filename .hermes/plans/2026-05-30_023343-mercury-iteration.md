# Mercury Iteration Implementation Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task. Keep this plan local unless Jonathan explicitly asks to commit it.

**Goal:** Add Mercury as the first orbiting planet in the C/raylib solar-system simulator while preserving the physics-first architecture and all Sun-foundation acceptance criteria.

**Architecture:** Extend the existing raylib-independent simulation model from one fixed Sun to two bodies: fixed Sun plus non-fixed Mercury. Keep all Mercury state in double-precision SI units, initialize Mercury at perihelion with a tangential velocity computed from the vis-viva equation, and let the existing Newtonian velocity-Verlet integrator move Mercury around the Sun. Rendering should remain a boundary concern: draw Mercury from the generic `Body` loop with a planet-specific color/visible-radius policy, not Mercury-specific rendering code scattered through `main.c`.

**Tech Stack:** C11, raylib, raymath only at render boundary, Makefile, C test binaries using `assert`/return codes.

---

## Progress Snapshot

| Field | Status |
|---|---|
| Repo | `/opt/data/github/jonathanperis/solar-system-simulator` |
| Remote | `origin https://github.com/jonathanperis/solar-system-simulator.git` |
| Branch/ref inspected | `main` at `83aa1ac` |
| Working tree before plan | Clean (`git status --short` returned no changes) |
| Prior plan consulted | `.hermes/plans/2026-05-30_020316-solar-system-foundation-sun.md` |
| Current implementation inspected | `src/sim/solar_system.*`, `src/sim/constants.h`, `src/sim/physics.*`, `src/render/renderer.*`, `src/main.c`, `tests/test_solar_system.c`, `README.md` |
| External evidence checked | NASA Mercury facts page for radius, average distance, 88-day period, speed, no moons/rings; local computation for perihelion state via vis-viva |
| Implementation status | Not started; planning only |
| Validation status | Not applicable yet |

Overall planning progress: Mercury iteration is specified. Implementation should stop after Mercury is initialized, tested, rendered, documented, and accepted; do not add Venus or other bodies in this iteration.

---

## Requirements and Non-Goals

### Requirements

- Add Mercury as the second celestial body.
- Preserve C-only implementation.
- Preserve raylib-independent simulation under `src/sim/`.
- Keep internal state in double-precision SI units.
- Use real physical Mercury mass and radius in simulation data.
- Initialize Mercury with a physically meaningful heliocentric orbit, not a hand-tuned circular animation.
- Use Newtonian gravity and the existing velocity-Verlet stepping.
- Render exactly two celestial bodies: Sun and Mercury.
- Show Mercury moving over time under physics integration.
- Keep `make`, `make test`, and bounded `make run` smoke passing.

### Non-Goals

- No Venus, Earth, Moon, or other body additions.
- No Mercury axial rotation, day/night, surface texture, temperature, magnetic field, or relativistic perihelion precession.
- No NASA Horizons ephemeris ingestion yet.
- No camera/UI overhaul beyond minimal overlay text adjustments.
- No dynamic body allocation or generalized asset/material system unless needed for the two-body milestone.

---

## Mercury Physical Data for This Iteration

Use these constants in `src/sim/constants.h` with comments naming the source/derivation.

| Quantity | Planned constant | Value | Notes |
|---|---:|---:|---|
| Mercury mass | `SOLAR_MERCURY_MASS_KG` | `3.3011e23` | Standard Mercury mass, kg |
| Mercury mean radius | `SOLAR_MERCURY_RADIUS_M` | `2439700.0` | NASA lists radius as about 2,440 km |
| Semi-major axis | `SOLAR_MERCURY_SEMI_MAJOR_AXIS_M` | `57909050000.0` | 57,909,050 km |
| Eccentricity | `SOLAR_MERCURY_ECCENTRICITY` | `0.205630` | Mercury's high orbital eccentricity |
| Perihelion distance | derived helper or constant | `46001212048.5 m` | `a * (1 - e)` |
| Aphelion distance | derived only in tests/docs if needed | `69816887951.5 m` | `a * (1 + e)` |
| Perihelion speed | `SOLAR_MERCURY_PERIHELION_SPEED_MPS` | about `58977.28405570045` | Computed with vis-viva using current `G * SOLAR_SUN_MASS_KG` |
| Orbital period check | test expectation | about `87.9677 days` | Kepler period from current constants; NASA public facts summarize as 88 days |

Derivation used for initial state:

```c
r_perihelion = a * (1.0 - e)
v_perihelion = sqrt((G * M_sun) * (2.0 / r_perihelion - 1.0 / a))
```

Initial coordinate convention:

- Sun starts at origin and remains `fixed == true`.
- Mercury starts at perihelion on +X:
  - `position_m = { SOLAR_MERCURY_PERIHELION_M, 0.0, 0.0 }`
  - `velocity_mps = { 0.0, 0.0, SOLAR_MERCURY_PERIHELION_SPEED_MPS }`
- This keeps the orbit in the XZ plane, matching the current camera/grid orientation.

---

## Design Notes

1. **Resize `SolarSystem.bodies` deliberately, not generically.**
   - Replace `Body bodies[1]` with a small named capacity constant, e.g. `#define SOLAR_SYSTEM_BODY_CAPACITY 2`.
   - This is the first real need for more than one body. Do not introduce heap allocation yet.

2. **Prefer a new initializer name over changing semantics silently.**
   - Keep `solar_system_create_sun_only()` for tests/regression if useful.
   - Add `solar_system_create_sun_mercury()` for the app's default scene.
   - This preserves the Sun-only milestone as a testable baseline.

3. **Mercury should not be fixed.**
   - `fixed == false`.
   - It should move from gravitational acceleration and its initial tangential velocity.

4. **Renderer should stay generic.**
   - The render loop should still iterate `system->body_count`.
   - Add a helper that maps `BodyKind`/name to color and minimum visible radius.
   - Avoid hard-coding Mercury draw calls in `main.c`.

5. **Tests should prove physics behavior, not visual animation.**
   - Window-free tests validate constants, initialization, acceleration direction, movement, and approximate orbital period stability.
   - The raylib smoke only proves the app launches with the new scene.

---

## Task Plan

### Task 1: Add Mercury constants and derived orbital helpers

**Objective:** Centralize Mercury's physical/orbital values in SI units.

**Files:**
- Modify: `src/sim/constants.h`
- Test: `tests/test_solar_system.c` or `tests/test_physics.c`

**Step 1: Write failing tests**

Add tests that reference constants that do not exist yet:

```c
static void test_mercury_constants_are_real_si_values(void)
{
    assert_close(SOLAR_MERCURY_MASS_KG, 3.3011e23, 3.3011e23 * 1e-12);
    assert_close(SOLAR_MERCURY_RADIUS_M, 2439700.0, 1e-6);
    assert_close(SOLAR_MERCURY_SEMI_MAJOR_AXIS_M, 57909050000.0, 1e-3);
    assert_close(SOLAR_MERCURY_ECCENTRICITY, 0.205630, 1e-12);
}
```

Also add a derived perihelion distance check once the API shape is chosen:

```c
static void test_mercury_perihelion_distance_is_derived_from_orbital_elements(void)
{
    double expected = SOLAR_MERCURY_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_MERCURY_ECCENTRICITY);
    assert_close(SOLAR_MERCURY_PERIHELION_M, expected, 1e-3);
}
```

**Step 2: Verify RED**

Run:

```bash
make test
```

Expected: FAIL because Mercury constants are undefined.

**Step 3: Implement constants**

Add to `src/sim/constants.h`:

```c
#define SOLAR_MERCURY_MASS_KG 3.3011e23
#define SOLAR_MERCURY_RADIUS_M 2439700.0
#define SOLAR_MERCURY_SEMI_MAJOR_AXIS_M 57909050000.0
#define SOLAR_MERCURY_ECCENTRICITY 0.205630
#define SOLAR_MERCURY_PERIHELION_M \
    (SOLAR_MERCURY_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_MERCURY_ECCENTRICITY))
#define SOLAR_MERCURY_PERIHELION_SPEED_MPS 58977.28405570045
```

**Step 4: Verify GREEN**

Run:

```bash
make test
```

Expected: PASS.

**Step 5: Commit**

```bash
git add src/sim/constants.h tests/test_solar_system.c tests/test_physics.c
git commit -m "feat: add Mercury physical constants"
```

---

### Task 2: Expand solar-system storage capacity safely

**Objective:** Allow the simulation to store Sun + Mercury without heap allocation.

**Files:**
- Modify: `src/sim/solar_system.h`
- Modify: `src/sim/solar_system.c`
- Test: `tests/test_solar_system.c`

**Step 1: Write failing test**

Add a test that expects a named body capacity:

```c
static void test_solar_system_capacity_supports_sun_and_mercury(void)
{
    assert(SOLAR_SYSTEM_BODY_CAPACITY == 2);
}
```

**Step 2: Verify RED**

Run:

```bash
make test
```

Expected: FAIL because `SOLAR_SYSTEM_BODY_CAPACITY` does not exist.

**Step 3: Implement minimal storage expansion**

In `src/sim/solar_system.h`:

```c
#define SOLAR_SYSTEM_BODY_CAPACITY 2

typedef struct SolarSystem {
    Body bodies[SOLAR_SYSTEM_BODY_CAPACITY];
    size_t body_count;
    double elapsed_seconds;
} SolarSystem;
```

Leave `solar_system_create_sun_only()` returning `body_count == 1`.

**Step 4: Verify GREEN**

Run:

```bash
make test
```

Expected: PASS.

**Step 5: Commit**

```bash
git add src/sim/solar_system.h src/sim/solar_system.c tests/test_solar_system.c
git commit -m "feat: expand solar system body capacity"
```

---

### Task 3: Add Mercury body initializer

**Objective:** Create a reusable body factory for Mercury's initial perihelion state.

**Files:**
- Modify: `src/sim/solar_system.h`
- Modify: `src/sim/solar_system.c`
- Test: `tests/test_solar_system.c`

**Step 1: Write failing tests**

Add tests for a new helper or initializer. Recommended API:

```c
Body solar_system_create_mercury_at_perihelion(void);
```

Test:

```c
static void test_mercury_body_starts_at_perihelion_with_tangential_velocity(void)
{
    Body mercury = solar_system_create_mercury_at_perihelion();

    assert(strcmp(mercury.name, "Mercury") == 0);
    assert(mercury.kind == BODY_KIND_PLANET);
    assert(!mercury.fixed);
    assert_close(mercury.mass_kg, SOLAR_MERCURY_MASS_KG, SOLAR_MERCURY_MASS_KG * 1e-12);
    assert_close(mercury.radius_m, SOLAR_MERCURY_RADIUS_M, 1e-6);
    assert_close(mercury.position_m.x, SOLAR_MERCURY_PERIHELION_M, 1e-3);
    assert_close(mercury.position_m.y, 0.0, 1e-12);
    assert_close(mercury.position_m.z, 0.0, 1e-12);
    assert_close(mercury.velocity_mps.x, 0.0, 1e-12);
    assert_close(mercury.velocity_mps.y, 0.0, 1e-12);
    assert_close(mercury.velocity_mps.z, SOLAR_MERCURY_PERIHELION_SPEED_MPS, 1e-6);
}
```

**Step 2: Verify RED**

Run:

```bash
make test
```

Expected: FAIL because `solar_system_create_mercury_at_perihelion()` is undefined.

**Step 3: Implement minimal helper**

In `src/sim/solar_system.h`:

```c
Body solar_system_create_mercury_at_perihelion(void);
```

In `src/sim/solar_system.c`:

```c
Body solar_system_create_mercury_at_perihelion(void)
{
    return body_create(
        "Mercury",
        BODY_KIND_PLANET,
        SOLAR_MERCURY_MASS_KG,
        SOLAR_MERCURY_RADIUS_M,
        (Vec3d){SOLAR_MERCURY_PERIHELION_M, 0.0, 0.0},
        (Vec3d){0.0, 0.0, SOLAR_MERCURY_PERIHELION_SPEED_MPS},
        false
    );
}
```

**Step 4: Verify GREEN**

Run:

```bash
make test
```

Expected: PASS.

**Step 5: Commit**

```bash
git add src/sim/solar_system.h src/sim/solar_system.c tests/test_solar_system.c
git commit -m "feat: initialize Mercury body state"
```

---

### Task 4: Add Sun + Mercury solar-system initializer

**Objective:** Create the default two-body simulation state while keeping Sun-only regression support.

**Files:**
- Modify: `src/sim/solar_system.h`
- Modify: `src/sim/solar_system.c`
- Test: `tests/test_solar_system.c`

**Step 1: Write failing tests**

Recommended API:

```c
SolarSystem solar_system_create_sun_mercury(void);
```

Test:

```c
static void test_sun_mercury_system_has_two_expected_bodies(void)
{
    SolarSystem system = solar_system_create_sun_mercury();

    assert(system.body_count == 2);
    assert(strcmp(system.bodies[0].name, "Sun") == 0);
    assert(strcmp(system.bodies[1].name, "Mercury") == 0);
    assert(system.bodies[0].fixed);
    assert(!system.bodies[1].fixed);
    assert(system.bodies[1].kind == BODY_KIND_PLANET);
}
```

**Step 2: Verify RED**

Run:

```bash
make test
```

Expected: FAIL because `solar_system_create_sun_mercury()` is undefined.

**Step 3: Implement initializer**

In `src/sim/solar_system.h`:

```c
SolarSystem solar_system_create_sun_mercury(void);
```

In `src/sim/solar_system.c`, avoid duplicated Sun construction by adding a small static helper if useful:

```c
static Body create_sun(void)
{
    return body_create(
        "Sun",
        BODY_KIND_STAR,
        SOLAR_SUN_MASS_KG,
        SOLAR_SUN_RADIUS_M,
        vec3d_zero(),
        vec3d_zero(),
        true
    );
}
```

Then:

```c
SolarSystem solar_system_create_sun_mercury(void)
{
    SolarSystem system = {
        .bodies = {
            create_sun(),
            solar_system_create_mercury_at_perihelion(),
        },
        .body_count = 2,
        .elapsed_seconds = 0.0,
    };

    return system;
}
```

**Step 4: Verify GREEN**

Run:

```bash
make test
```

Expected: PASS.

**Step 5: Commit**

```bash
git add src/sim/solar_system.h src/sim/solar_system.c tests/test_solar_system.c
git commit -m "feat: create Sun and Mercury system"
```

---

### Task 5: Prove Mercury moves under Sun gravity

**Objective:** Add focused physics/regression tests that the two-body simulation produces physically plausible motion.

**Files:**
- Modify: `tests/test_physics.c`
- Modify: `tests/test_solar_system.c`
- Possibly modify: `src/sim/physics.c` only if a real bug is exposed

**Step 1: Write failing/plausibility tests**

Add tests for gravity direction and one-day motion.

```c
static void test_mercury_acceleration_points_toward_sun_at_perihelion(void)
{
    SolarSystem system = solar_system_create_sun_mercury();
    physics_compute_accelerations(system.bodies, system.body_count);

    Body mercury = system.bodies[1];
    assert(mercury.acceleration_mps2.x < 0.0);
    assert_close(mercury.acceleration_mps2.y, 0.0, 1e-18);
    assert_close(mercury.acceleration_mps2.z, 0.0, 1e-18);
}
```

```c
static void test_mercury_moves_after_one_day_while_sun_stays_fixed(void)
{
    SolarSystem system = solar_system_create_sun_mercury();
    Vec3d initial_mercury_position = system.bodies[1].position_m;

    solar_system_step(&system, SOLAR_DAY_SECONDS);

    assert_close(system.bodies[0].position_m.x, 0.0, 1e-12);
    assert(system.bodies[1].position_m.x < initial_mercury_position.x);
    assert(system.bodies[1].position_m.z > initial_mercury_position.z);
}
```

These tests should pass with the existing physics implementation once Mercury is present. If they fail, inspect whether the failure is due to sign convention, initializer state, or a real integrator bug.

**Step 2: Verify**

Run:

```bash
make test
```

Expected: PASS once prior tasks are complete. If RED does not fail because implementation already satisfies it, this is acceptable as a regression test after adding the initializer; document it in the task notes.

**Step 3: Commit**

```bash
git add tests/test_physics.c tests/test_solar_system.c src/sim/physics.c
git commit -m "test: cover Mercury orbital motion"
```

---

### Task 6: Add approximate Mercury orbital-period regression

**Objective:** Guard against future integrator/sign/constant regressions with a coarse full-orbit check.

**Files:**
- Modify: `tests/test_solar_system.c`

**Step 1: Write test**

Add a coarse simulation test using a fixed timestep, e.g. 6 hours. Do not demand perfect ephemeris accuracy; this is a stability/plausibility guard.

```c
static void test_mercury_roughly_returns_after_one_orbit(void)
{
    SolarSystem system = solar_system_create_sun_mercury();
    Vec3d initial = system.bodies[1].position_m;
    const double dt = 6.0 * 60.0 * 60.0;
    const int steps = (int)((87.9677 * SOLAR_DAY_SECONDS) / dt);

    for (int i = 0; i < steps; ++i) {
        solar_system_step(&system, dt);
    }

    double distance_from_initial = vec3d_length(vec3d_sub(system.bodies[1].position_m, initial));
    assert(distance_from_initial < 0.08 * SOLAR_AU_METERS);
}
```

**Important:** Keep tolerance intentionally loose. This is not an ephemeris test; it checks that Mercury is bound and approximately periodic with the configured constants.

**Step 2: Verify**

Run:

```bash
make test
```

Expected: PASS within normal test runtime.

If runtime is too high, increase timestep to 12 hours and loosen tolerance carefully; do not remove the behavior check.

**Step 3: Commit**

```bash
git add tests/test_solar_system.c
git commit -m "test: add Mercury orbit regression"
```

---

### Task 7: Render Mercury distinctly from the Sun

**Objective:** Draw Mercury as the second celestial body with a visible but physically separated display radius.

**Files:**
- Modify: `src/render/renderer.c`
- Possibly modify: `src/sim/constants.h`
- No window-opening unit test required; verified by build/smoke.

**Step 1: Inspect current render loop**

Current `renderer_draw_solar_system()` already iterates all bodies. Keep that generic behavior.

**Step 2: Implement minimal render policy**

Add helpers similar to:

```c
static Color body_render_color(const Body *body)
{
    if (body->kind == BODY_KIND_STAR) {
        return GOLD;
    }

    if (body->name != NULL && TextIsEqual(body->name, "Mercury")) {
        return GRAY;
    }

    return LIGHTGRAY;
}
```

Because `TextIsEqual` is raylib text utility, either use it inside renderer or use a small local string comparison with `<string.h>`. Prefer `<string.h>` to avoid coupling color logic to raylib text helpers:

```c
#include <string.h>
```

Add a minimum visible radius policy that keeps Mercury visible without corrupting physical radius:

```c
static float body_min_visible_radius(const Body *body)
{
    return body->kind == BODY_KIND_STAR ? 0.5f : 0.12f;
}
```

Then clamp against that body-specific minimum.

**Step 3: Verify**

Run:

```bash
make
make test
```

Expected: build/tests pass.

**Step 4: Commit**

```bash
git add src/render/renderer.c src/sim/constants.h
git commit -m "feat: render Mercury distinctly"
```

---

### Task 8: Switch app default scene to Sun + Mercury and update overlay

**Objective:** Make the app launch the Mercury iteration by default.

**Files:**
- Modify: `src/main.c`

**Step 1: Update initializer**

Change:

```c
SolarSystem system = solar_system_create_sun_only();
```

to:

```c
SolarSystem system = solar_system_create_sun_mercury();
```

**Step 2: Update overlay title/text**

Change title overlay to say:

```c
DrawText("Solar System Simulator - Milestone 2: Mercury", 20, 20, 20, RAYWHITE);
```

Optionally add:

```c
DrawText("Bodies: Sun + Mercury", 20, 150, 18, RAYWHITE);
```

Do not add controls/UI menus in this iteration.

**Step 3: Verify**

Run:

```bash
make
make test
```

Expected: build/tests pass.

**Step 4: Commit**

```bash
git add src/main.c
git commit -m "feat: launch Sun and Mercury simulation"
```

---

### Task 9: Document Mercury milestone

**Objective:** Update docs to describe the new body, physics assumptions, and validation commands.

**Files:**
- Modify: `README.md`

**Step 1: Update README**

Add or modify sections:

- Rename current milestone section to include Mercury:
  - `Milestone 2: Mercury`
- State current behavior:
  - renders Sun and Mercury
  - Mercury starts at perihelion
  - Mercury moves due to Newtonian gravity from the Sun
- Add Mercury data:
  - mass, radius, semi-major axis, eccentricity, initial perihelion speed
- Keep future roadmap but remove Mercury as next planned item or mark it complete:
  1. Venus
  2. Earth
  3. Moon
  ...

**Step 2: Verify README accuracy**

Check that README does not claim:

- accurate ephemerides;
- relativistic precession;
- visual scale equals physical radius;
- any body beyond Sun and Mercury exists.

**Step 3: Commit**

```bash
git add README.md
git commit -m "docs: document Mercury iteration"
```

---

### Task 10: Final acceptance checks and review

**Objective:** Confirm the Mercury iteration meets all acceptance criteria before reporting.

**Files:**
- No intended source changes unless review finds issues.

**Step 1: Run full checks**

```bash
make clean
make
make test
set +e
timeout 3s make run > /tmp/solar-mercury-run.log 2>&1
run_code=$?
set -e
if [ "$run_code" -ne 124 ] && [ "$run_code" -ne 0 ]; then
  cat /tmp/solar-mercury-run.log
  exit "$run_code"
fi
grep -E 'Initializing raylib|Device initialized successfully|PLATFORM: .*Initialized successfully' /tmp/solar-mercury-run.log
git diff --check
```

Expected:

- `make` succeeds.
- `make test` succeeds.
- bounded `make run` initializes raylib successfully.
- `git diff --check` has no whitespace errors.

**Step 2: Dispatch final review**

Use subagent-driven-development final review pattern:

- Spec compliance reviewer checks every requirement in this plan.
- Code quality reviewer checks simplicity, C style, test coverage, no scope creep.

**Step 3: Fix any review issues**

If review finds blocking issues, fix them with focused commits and rerun checks.

**Step 4: Push when clean**

```bash
git status --short
git push origin main
git rev-list --left-right --count HEAD...origin/main
```

Expected after push: `0 0`.

---

## Acceptance Criteria for Mercury Iteration

- `make` builds `build/solar-system-simulator`.
- `make test` runs all C test binaries and passes.
- bounded `make run` initializes raylib successfully; on a desktop, `make run` opens the window.
- The scene renders exactly two celestial bodies: Sun and Mercury.
- Sun remains fixed at the origin.
- Mercury uses real mass and radius in simulation data.
- Mercury initial position uses perihelion distance derived from semi-major axis and eccentricity.
- Mercury initial velocity uses a tangential perihelion speed derived from the vis-viva equation.
- Mercury is non-fixed and moves under Newtonian gravity.
- Tests prove Mercury acceleration points toward the Sun at perihelion.
- Tests prove Mercury moves after stepping and remains approximately bound/periodic over one orbit.
- Rendering scale remains isolated from simulation units.
- README documents Mercury scope, assumptions, commands, and next planned body iteration.

---

## Risks and Mitigations

| Risk | Mitigation |
|---|---|
| Full-orbit test becomes brittle | Use coarse tolerance and state it is a plausibility regression, not an ephemeris oracle |
| Mercury's physical radius is invisible at AU render scale | Keep true radius in simulation; use explicit per-body minimum visible radius in renderer |
| Orbit appears too fast/slow visually | Keep current time scale unless it prevents visual verification; document time scale rather than changing controls |
| Fixed Sun introduces non-barycentric approximation | Accept for this milestone; document fixed-Sun heliocentric mode as intentional |
| Perihelion speed constant drifts from `G`/Sun mass if those change | Prefer deriving speed in a helper if the implementation remains simple, or add a test that recomputes expected speed from current constants |
| Scope creep into ephemerides/relativity | Defer NASA Horizons/J2000/relativistic corrections to later physics-hardening milestones |

---

## Open Questions for Later Iterations

These do not block Mercury:

1. Should Venus use the same perihelion initialization pattern or start from a common epoch state vector?
2. When enough planets exist, should the Sun become non-fixed and the system be initialized barycentrically?
3. Should orbital traces be added as visual diagnostics after multiple bodies exist?
4. Should tests start tracking approximate energy conservation once at least two non-fixed bodies are present?

---

## Implementation Handoff

Plan complete for the Mercury iteration. Execute using subagent-driven-development: fresh implementation subagent per task, spec review first, code-quality review second, and final integration review. Stop after Mercury is implemented and accepted; do not add Venus or any other body in this iteration.
