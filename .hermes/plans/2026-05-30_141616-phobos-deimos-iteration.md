# Phobos and Deimos Iteration Implementation Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task.

**Goal:** Add Mars's two natural satellites, Phobos and Deimos, as the next physics-first milestone after Mars.

**Architecture:** Keep all physical state in `src/sim/` as double-precision SI units. Add Phobos/Deimos as Mars-relative moon factories whose absolute heliocentric state is `Mars state + satellite relative state`, then update app/render/docs/tests around an eight-body scene. Rendering may use illustrative parent-relative separation for tiny Martian moons, but must not mutate simulation state.

**Tech Stack:** C11, raylib, custom `Vec3d`, SI-unit Newtonian gravity, velocity-Verlet integrator, Makefile C test binaries.

---

## Planning Progress Snapshot

| Field | Status |
|---|---|
| Repo/path inspected | `/opt/data/github/jonathanperis/solar-system-simulator` |
| Branch/ref inspected | `main` at `24761db` (`fix: keep full motion trail history`) |
| Remote sync | `HEAD...origin/main = 0 0` before writing this plan |
| Working tree before plan | Clean |
| Prior plans consulted | Mars plan plus current source/tests; `.hermes/plans/2026-05-30_133326-mars-iteration.md` is the immediate predecessor |
| External data consulted | NASA Mars moons pages; NASA/JPL SSD satellite physical-parameter table; NASA/PDS Deimos context; NSSDC-derived Mars satellite orbit facts from indexed NASA/NSSDC mirror |
| Implementation status | Not started; this document is a plan only |
| Validation status | Verification commands listed below; no code changed yet |

Overall planning progress: requirements are clear enough to implement. The main risk is the short Phobos orbital period relative to the app's one-day-per-second time scale; this plan includes a small fixed app-side substep policy so the physics stays stable without changing `src/sim/` semantics.

---

## Acceptance Criteria

This iteration is complete only when all criteria below pass:

1. `src/sim/` still has no raylib dependency.
2. The simulation supports exactly eight bodies in the main milestone scene:
   1. Sun
   2. Mercury
   3. Venus
   4. Earth
   5. Moon
   6. Mars
   7. Phobos
   8. Deimos
3. Phobos and Deimos have SI constants for mass, mean radius, semi-major axis, eccentricity, periapsis/periareion, apoapsis/apoareion, and periapsis speed.
4. Phobos and Deimos are `BODY_KIND_MOON`, not fixed bodies, and are initialized relative to Mars.
5. Their absolute initial state is `Mars.position + relative_offset` and `Mars.velocity + relative_velocity`.
6. Phobos and Deimos use vis-viva speed around Mars using `G * (MarsMass + MoonMass)`.
7. Tests prove parent-relative initial position/velocity, prograde movement, and bounded Mars-moon separation over at least one coarse orbital period.
8. The main app uses the eight-body scene.
9. Camera focus cycles through Phobos and Deimos automatically.
10. Motion trails record Phobos and Deimos and never extinguish during the app run.
11. Renderer gives Phobos and Deimos explicit colors and keeps them visible/readable in illustrative mode without changing physics state.
12. The adaptive grid still covers the farthest rendered body.
13. README and overlay document Milestone 7: Phobos + Deimos.
14. Verification passes:
    - `make clean`
    - `make`
    - `make test`
    - bounded raylib smoke: `timeout 3s make run`
    - `git diff --check`
15. Commit and push the finished implementation at the end of the iteration.

---

## Source Data to Use

Use these values for the first implementation pass. Keep comments or README notes clear that these are simplified milestone values, not JPL ephemeris state vectors.

### Physical Parameters

From NASA/JPL SSD planetary satellite physical parameters:

- Phobos:
  - `GM = 0.0007087 km^3/s^2`
  - mean radius `11.08 km`
  - mass derived as `GM * 1e9 / SOLAR_G = 1.061834199841182e16 kg`
- Deimos:
  - `GM = 0.0000962 km^3/s^2`
  - mean radius `6.2 km`
  - mass derived as `GM * 1e9 / SOLAR_G = 1.441349654645431e15 kg`

Use derived mass constants to match the current simulator model, which stores body mass and computes gravity from `SOLAR_G`.

### Orbital Parameters

Use simple Mars-relative osculating-style milestone values from NASA/NSSDC-derived Mars satellite facts and NASA/PDS summaries:

- Phobos:
  - mean distance / semi-major axis from Mars center: `9,377 km` -> `9,377,000 m`
  - eccentricity: `0.0151`
  - sidereal period reference: about `0.31891 days` (`7 h 39 min`)
  - periareion: `a * (1 - e) = 9,235,407.3 m`
  - apoareion: `a * (1 + e) = 9,518,592.7 m`
  - periareion speed with current constants: about `2170.0160220561597 m/s`
- Deimos:
  - mean distance / semi-major axis from Mars center: `23,460 km` -> `23,460,000 m`
  - eccentricity: `0.00033`
  - sidereal period reference: about `1.26244 days` / NASA narrative: about `30 hours`
  - periareion: `23,452,258.2 m`
  - apoareion: `23,467,741.8 m`
  - periareion speed with current constants: about `1351.8106494404324 m/s`

Do not add Phobos orbital decay, Mars rotation, tidal effects, irregular shape models, or surface features in this iteration.

---

## Proposed Coordinate Layout

Current scene uses deterministic, separated initial axes:

- Mercury: `+X`, `+Z` velocity.
- Venus: `-X`, `-Z` velocity.
- Earth: `+Z`, `-X` velocity.
- Moon: Earth-relative `+X`, relative `+Z` velocity.
- Mars: `-Z`, `+X` velocity.

For this iteration:

- Phobos: Mars-relative `+X`, relative `+Y` velocity.
- Deimos: Mars-relative `-X`, relative `-Y` velocity.

Reason: both Martian moons start near Mars without overlapping each other along the same radial line, and the `X/Y` local plane is still perpendicular to neither Mars's heliocentric tangent nor existing inner-system axes in a way that changes physics semantics. This is a deterministic visualization-friendly initialization, not a real Mars-equatorial ephemeris.

If implementation wants both moons in the same Mars-equatorial plane, use the same `+X/+Y` plane for Phobos and `-X/-Y` for Deimos as above.

---

## Task 1: Add Phobos and Deimos constants tests

**Objective:** Establish RED tests for physical/orbital constants before adding production constants.

**Files:**
- Modify: `tests/test_solar_system.c`
- Later modify: `src/sim/constants.h`

**Step 1: Write failing tests**

Add test functions near the Mars constants tests:

```c
static void test_phobos_constants_are_real_si_values(void)
{
    assert_close(SOLAR_PHOBOS_MASS_KG, 1.061834199841182e16, 1.061834199841182e16 * 1e-12);
    assert_close(SOLAR_PHOBOS_RADIUS_M, 11080.0, 1e-9);
    assert_close(SOLAR_PHOBOS_SEMI_MAJOR_AXIS_M, 9377000.0, 1e-6);
    assert_close(SOLAR_PHOBOS_ECCENTRICITY, 0.0151, 1e-12);
}

static void test_deimos_constants_are_real_si_values(void)
{
    assert_close(SOLAR_DEIMOS_MASS_KG, 1.441349654645431e15, 1.441349654645431e15 * 1e-12);
    assert_close(SOLAR_DEIMOS_RADIUS_M, 6200.0, 1e-9);
    assert_close(SOLAR_DEIMOS_SEMI_MAJOR_AXIS_M, 23460000.0, 1e-6);
    assert_close(SOLAR_DEIMOS_ECCENTRICITY, 0.00033, 1e-12);
}

static void test_phobos_periareion_distance_is_derived_from_orbital_elements(void)
{
    double expected = SOLAR_PHOBOS_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_PHOBOS_ECCENTRICITY);
    assert_close(SOLAR_PHOBOS_PERIAREION_M, expected, 1e-6);
}

static void test_deimos_periareion_distance_is_derived_from_orbital_elements(void)
{
    double expected = SOLAR_DEIMOS_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_DEIMOS_ECCENTRICITY);
    assert_close(SOLAR_DEIMOS_PERIAREION_M, expected, 1e-6);
}

static void test_phobos_periareion_speed_matches_mars_phobos_vis_viva(void)
{
    double mu = SOLAR_G * (SOLAR_MARS_MASS_KG + SOLAR_PHOBOS_MASS_KG);
    double expected = sqrt(mu * ((2.0 / SOLAR_PHOBOS_PERIAREION_M) - (1.0 / SOLAR_PHOBOS_SEMI_MAJOR_AXIS_M)));
    assert_close(SOLAR_PHOBOS_PERIAREION_SPEED_MPS, expected, 1e-9);
}

static void test_deimos_periareion_speed_matches_mars_deimos_vis_viva(void)
{
    double mu = SOLAR_G * (SOLAR_MARS_MASS_KG + SOLAR_DEIMOS_MASS_KG);
    double expected = sqrt(mu * ((2.0 / SOLAR_DEIMOS_PERIAREION_M) - (1.0 / SOLAR_DEIMOS_SEMI_MAJOR_AXIS_M)));
    assert_close(SOLAR_DEIMOS_PERIAREION_SPEED_MPS, expected, 1e-9);
}
```

Register all six tests in `main()` immediately after the Mars constant tests.

**Step 2: Run RED**

Run:

```bash
make build/tests/test_solar_system
```

Expected: FAIL at compile time because `SOLAR_PHOBOS_*` and `SOLAR_DEIMOS_*` constants do not exist.

**Step 3: Implement constants**

Add to `src/sim/constants.h` after Mars constants:

```c
#define SOLAR_PHOBOS_MASS_KG 1.061834199841182e16
#define SOLAR_PHOBOS_RADIUS_M 11080.0
#define SOLAR_PHOBOS_SEMI_MAJOR_AXIS_M 9377000.0
#define SOLAR_PHOBOS_ECCENTRICITY 0.0151
#define SOLAR_PHOBOS_PERIAREION_M \
    (SOLAR_PHOBOS_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_PHOBOS_ECCENTRICITY))
#define SOLAR_PHOBOS_APOAREION_M \
    (SOLAR_PHOBOS_SEMI_MAJOR_AXIS_M * (1.0 + SOLAR_PHOBOS_ECCENTRICITY))
#define SOLAR_PHOBOS_PERIAREION_SPEED_MPS \
    (sqrt(SOLAR_G * (SOLAR_MARS_MASS_KG + SOLAR_PHOBOS_MASS_KG) * \
        ((2.0 / SOLAR_PHOBOS_PERIAREION_M) - (1.0 / SOLAR_PHOBOS_SEMI_MAJOR_AXIS_M))))

#define SOLAR_DEIMOS_MASS_KG 1.441349654645431e15
#define SOLAR_DEIMOS_RADIUS_M 6200.0
#define SOLAR_DEIMOS_SEMI_MAJOR_AXIS_M 23460000.0
#define SOLAR_DEIMOS_ECCENTRICITY 0.00033
#define SOLAR_DEIMOS_PERIAREION_M \
    (SOLAR_DEIMOS_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_DEIMOS_ECCENTRICITY))
#define SOLAR_DEIMOS_APOAREION_M \
    (SOLAR_DEIMOS_SEMI_MAJOR_AXIS_M * (1.0 + SOLAR_DEIMOS_ECCENTRICITY))
#define SOLAR_DEIMOS_PERIAREION_SPEED_MPS \
    (sqrt(SOLAR_G * (SOLAR_MARS_MASS_KG + SOLAR_DEIMOS_MASS_KG) * \
        ((2.0 / SOLAR_DEIMOS_PERIAREION_M) - (1.0 / SOLAR_DEIMOS_SEMI_MAJOR_AXIS_M))))
```

**Step 4: Run GREEN**

Run:

```bash
make build/tests/test_solar_system && build/tests/test_solar_system
```

Expected: PASS once only constants-related tests are added and satisfied.

---

## Task 2: Add moon factory tests and body capacity

**Objective:** Prove Phobos/Deimos can be created as Mars-relative moons before implementing factories.

**Files:**
- Modify: `tests/test_solar_system.c`
- Later modify: `src/sim/solar_system.h`
- Later modify: `src/sim/solar_system.c`

**Step 1: Write failing tests**

Replace capacity test with eight-body expectation:

```c
static void test_solar_system_capacity_supports_sun_mercury_venus_earth_moon_mars_phobos_and_deimos(void)
{
    assert(SOLAR_SYSTEM_BODY_CAPACITY == 8);
}
```

Add factory tests after `test_mars_body_starts_at_perihelion_with_tangential_velocity`:

```c
static void test_phobos_body_starts_at_mars_relative_periareion_with_tangential_velocity(void)
{
    Body mars = solar_system_create_mars_at_perihelion();
    Body phobos = solar_system_create_phobos_at_periareion_near_mars(&mars);
    Vec3d mars_to_phobos = vec3d_sub(phobos.position_m, mars.position_m);
    Vec3d relative_velocity = vec3d_sub(phobos.velocity_mps, mars.velocity_mps);

    assert(strcmp(phobos.name, "Phobos") == 0);
    assert(phobos.kind == BODY_KIND_MOON);
    assert(!phobos.fixed);
    assert_close(phobos.mass_kg, SOLAR_PHOBOS_MASS_KG, SOLAR_PHOBOS_MASS_KG * 1e-12);
    assert_close(phobos.radius_m, SOLAR_PHOBOS_RADIUS_M, 1e-9);
    assert_close(mars_to_phobos.x, SOLAR_PHOBOS_PERIAREION_M, 1e-6);
    assert_close(mars_to_phobos.y, 0.0, 1e-12);
    assert_close(mars_to_phobos.z, 0.0, 1e-12);
    assert_close(relative_velocity.x, 0.0, 1e-12);
    assert_close(relative_velocity.y, SOLAR_PHOBOS_PERIAREION_SPEED_MPS, 1e-9);
    assert_close(relative_velocity.z, 0.0, 1e-12);
}

static void test_deimos_body_starts_at_mars_relative_periareion_with_tangential_velocity(void)
{
    Body mars = solar_system_create_mars_at_perihelion();
    Body deimos = solar_system_create_deimos_at_periareion_near_mars(&mars);
    Vec3d mars_to_deimos = vec3d_sub(deimos.position_m, mars.position_m);
    Vec3d relative_velocity = vec3d_sub(deimos.velocity_mps, mars.velocity_mps);

    assert(strcmp(deimos.name, "Deimos") == 0);
    assert(deimos.kind == BODY_KIND_MOON);
    assert(!deimos.fixed);
    assert_close(deimos.mass_kg, SOLAR_DEIMOS_MASS_KG, SOLAR_DEIMOS_MASS_KG * 1e-12);
    assert_close(deimos.radius_m, SOLAR_DEIMOS_RADIUS_M, 1e-9);
    assert_close(mars_to_deimos.x, -SOLAR_DEIMOS_PERIAREION_M, 1e-6);
    assert_close(mars_to_deimos.y, 0.0, 1e-12);
    assert_close(mars_to_deimos.z, 0.0, 1e-12);
    assert_close(relative_velocity.x, 0.0, 1e-12);
    assert_close(relative_velocity.y, -SOLAR_DEIMOS_PERIAREION_SPEED_MPS, 1e-9);
    assert_close(relative_velocity.z, 0.0, 1e-12);
}
```

Register the tests in `main()`.

**Step 2: Run RED**

Run:

```bash
make build/tests/test_solar_system
```

Expected: FAIL at compile time because factories and capacity do not exist.

**Step 3: Implement factories**

In `src/sim/solar_system.h`:

```c
#define SOLAR_SYSTEM_BODY_CAPACITY 8

Body solar_system_create_phobos_at_periareion_near_mars(const Body *mars);
Body solar_system_create_deimos_at_periareion_near_mars(const Body *mars);
```

In `src/sim/solar_system.c` after `solar_system_create_mars_at_perihelion`:

```c
Body solar_system_create_phobos_at_periareion_near_mars(const Body *mars)
{
    Vec3d offset_m = {SOLAR_PHOBOS_PERIAREION_M, 0.0, 0.0};
    Vec3d relative_velocity_mps = {0.0, SOLAR_PHOBOS_PERIAREION_SPEED_MPS, 0.0};

    return body_create(
        "Phobos",
        BODY_KIND_MOON,
        SOLAR_PHOBOS_MASS_KG,
        SOLAR_PHOBOS_RADIUS_M,
        vec3d_add(mars->position_m, offset_m),
        vec3d_add(mars->velocity_mps, relative_velocity_mps),
        false
    );
}

Body solar_system_create_deimos_at_periareion_near_mars(const Body *mars)
{
    Vec3d offset_m = {-SOLAR_DEIMOS_PERIAREION_M, 0.0, 0.0};
    Vec3d relative_velocity_mps = {0.0, -SOLAR_DEIMOS_PERIAREION_SPEED_MPS, 0.0};

    return body_create(
        "Deimos",
        BODY_KIND_MOON,
        SOLAR_DEIMOS_MASS_KG,
        SOLAR_DEIMOS_RADIUS_M,
        vec3d_add(mars->position_m, offset_m),
        vec3d_add(mars->velocity_mps, relative_velocity_mps),
        false
    );
}
```

**Step 4: Run GREEN**

Run:

```bash
make build/tests/test_solar_system && build/tests/test_solar_system
```

Expected: PASS or progress to the next missing scene-factory assertions.

---

## Task 3: Add eight-body scene factory tests

**Objective:** Add an explicit full milestone scene without deleting older scene factories.

**Files:**
- Modify: `tests/test_solar_system.c`
- Later modify: `src/sim/solar_system.h`
- Later modify: `src/sim/solar_system.c`

**Step 1: Write failing test**

Add after the six-body Mars scene test:

```c
static void test_sun_mercury_venus_earth_moon_mars_phobos_deimos_system_has_eight_expected_bodies(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos();

    assert(system.body_count == 8);
    assert(strcmp(system.bodies[0].name, "Sun") == 0);
    assert(strcmp(system.bodies[1].name, "Mercury") == 0);
    assert(strcmp(system.bodies[2].name, "Venus") == 0);
    assert(strcmp(system.bodies[3].name, "Earth") == 0);
    assert(strcmp(system.bodies[4].name, "Moon") == 0);
    assert(strcmp(system.bodies[5].name, "Mars") == 0);
    assert(strcmp(system.bodies[6].name, "Phobos") == 0);
    assert(strcmp(system.bodies[7].name, "Deimos") == 0);
    assert(system.bodies[0].fixed);
    assert(!system.bodies[6].fixed);
    assert(!system.bodies[7].fixed);
    assert(system.bodies[6].kind == BODY_KIND_MOON);
    assert(system.bodies[7].kind == BODY_KIND_MOON);
}
```

Register in `main()`.

**Step 2: Run RED**

Run:

```bash
make build/tests/test_solar_system
```

Expected: FAIL because the scene factory does not exist.

**Step 3: Implement scene factory**

In `src/sim/solar_system.h`:

```c
SolarSystem solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos(void);
```

In `src/sim/solar_system.c` after the Mars scene factory:

```c
SolarSystem solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos(void)
{
    Body earth = solar_system_create_earth_at_perihelion();
    Body mars = solar_system_create_mars_at_perihelion();

    SolarSystem system = {
        .bodies = {
            create_sun(),
            solar_system_create_mercury_at_perihelion(),
            solar_system_create_venus_at_perihelion(),
            earth,
            solar_system_create_moon_at_perigee_near_earth(&earth),
            mars,
            solar_system_create_phobos_at_periareion_near_mars(&mars),
            solar_system_create_deimos_at_periareion_near_mars(&mars),
        },
        .body_count = 8,
        .elapsed_seconds = 0.0,
    };

    return system;
}
```

**Step 4: Run GREEN**

Run:

```bash
make build/tests/test_solar_system && build/tests/test_solar_system
```

Expected: PASS or progress to the next movement tests.

---

## Task 4: Add parent-relative motion/orbit tests

**Objective:** Prove Phobos and Deimos move prograde around Mars and remain near Mars over coarse orbital periods.

**Files:**
- Modify: `tests/test_solar_system.c`

**Step 1: Write failing/passing movement tests against the new implementation**

Add tests near existing Moon/Mars movement tests:

```c
static void test_phobos_moves_prograde_relative_to_mars_after_small_step(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos();
    Vec3d initial_relative = vec3d_sub(system.bodies[6].position_m, system.bodies[5].position_m);

    solar_system_step(&system, 5.0 * 60.0);

    Vec3d relative = vec3d_sub(system.bodies[6].position_m, system.bodies[5].position_m);
    assert(relative.x < initial_relative.x);
    assert(relative.y > initial_relative.y);
}

static void test_deimos_moves_prograde_relative_to_mars_after_small_step(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos();
    Vec3d initial_relative = vec3d_sub(system.bodies[7].position_m, system.bodies[5].position_m);

    solar_system_step(&system, 5.0 * 60.0);

    Vec3d relative = vec3d_sub(system.bodies[7].position_m, system.bodies[5].position_m);
    assert(relative.x > initial_relative.x);
    assert(relative.y < initial_relative.y);
}

static void test_phobos_remains_near_mars_after_one_orbit(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos();
    const double dt_seconds = 2.0 * 60.0;
    const double mu = SOLAR_G * (SOLAR_MARS_MASS_KG + SOLAR_PHOBOS_MASS_KG);
    const double period_seconds = 2.0 * acos(-1.0) * sqrt(
        (SOLAR_PHOBOS_SEMI_MAJOR_AXIS_M * SOLAR_PHOBOS_SEMI_MAJOR_AXIS_M * SOLAR_PHOBOS_SEMI_MAJOR_AXIS_M) / mu
    );
    const int steps = (int)(period_seconds / dt_seconds);

    for (int i = 0; i < steps; ++i) {
        solar_system_step(&system, dt_seconds);
    }

    Vec3d mars_to_phobos = vec3d_sub(system.bodies[6].position_m, system.bodies[5].position_m);
    double distance = vec3d_length(mars_to_phobos);
    assert(distance > 0.80 * SOLAR_PHOBOS_PERIAREION_M);
    assert(distance < 1.30 * SOLAR_PHOBOS_APOAREION_M);
}

static void test_deimos_remains_near_mars_after_one_orbit(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos();
    const double dt_seconds = 5.0 * 60.0;
    const double mu = SOLAR_G * (SOLAR_MARS_MASS_KG + SOLAR_DEIMOS_MASS_KG);
    const double period_seconds = 2.0 * acos(-1.0) * sqrt(
        (SOLAR_DEIMOS_SEMI_MAJOR_AXIS_M * SOLAR_DEIMOS_SEMI_MAJOR_AXIS_M * SOLAR_DEIMOS_SEMI_MAJOR_AXIS_M) / mu
    );
    const int steps = (int)(period_seconds / dt_seconds);

    for (int i = 0; i < steps; ++i) {
        solar_system_step(&system, dt_seconds);
    }

    Vec3d mars_to_deimos = vec3d_sub(system.bodies[7].position_m, system.bodies[5].position_m);
    double distance = vec3d_length(mars_to_deimos);
    assert(distance > 0.80 * SOLAR_DEIMOS_PERIAREION_M);
    assert(distance < 1.30 * SOLAR_DEIMOS_APOAREION_M);
}
```

Register in `main()`.

**Step 2: Run tests**

Run:

```bash
make build/tests/test_solar_system && build/tests/test_solar_system
```

Expected: PASS if previous tasks are correct. If the Phobos orbit test fails due to timestep sensitivity, do **not** weaken it first; reduce the test timestep to `60s` and inspect relative energy/distance before changing physics.

---

## Task 5: Add app-side physics substeps for short Martian moon periods

**Objective:** Keep the live app stable when one real second equals one simulation day and Phobos orbits Mars in about 7.65 hours.

**Files:**
- Modify: `src/main.c`
- Test: existing `make` and smoke test

**Rationale:** `solar_system_step()` should remain a simple integrator entry point. The app currently sends roughly one simulation day per real second into the integrator. With a 60 FPS frame, each step is roughly 1440 simulation seconds, only about 19 steps per Phobos orbit. A small app-side fixed substep improves visual/physical stability without complicating `src/sim/`.

**Implementation shape:**

Add near the top of `src/main.c`:

```c
#define SOLAR_APP_MAX_PHYSICS_STEP_SECONDS (5.0 * 60.0)
```

Add helper:

```c
static void step_system_with_substeps(SolarSystem *system, double dt_seconds)
{
    while (dt_seconds > SOLAR_APP_MAX_PHYSICS_STEP_SECONDS) {
        solar_system_step(system, SOLAR_APP_MAX_PHYSICS_STEP_SECONDS);
        dt_seconds -= SOLAR_APP_MAX_PHYSICS_STEP_SECONDS;
    }

    if (dt_seconds > 0.0) {
        solar_system_step(system, dt_seconds);
    }
}
```

Replace in the main loop:

```c
solar_system_step(&system, (double)frame_time * time_scale);
```

with:

```c
step_system_with_substeps(&system, (double)frame_time * time_scale);
```

Keep `body_trails_record_system(&trails, &system);` once per rendered frame so trails do not explode with substep-only points.

**Verification:**

Run:

```bash
make
```

Expected: app builds.

---

## Task 6: Wire the eight-body scene into the app and trails tests

**Objective:** Make the app render Phobos/Deimos and ensure trails record them forever.

**Files:**
- Modify: `src/main.c`
- Modify: `tests/test_body_trails.c`

**Step 1: Update trails tests first**

In `tests/test_body_trails.c`, replace the main scene factory with `solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos()` in the record/append/full-history tests.

Update assertions:

```c
assert(body_trails_point_count(&trails, 6) == 1);
assert(body_trails_point_count(&trails, 7) == 1);
assert_vec3d_equal(body_trails_point_at(&trails, 6, 0), system.bodies[6].position_m);
assert_vec3d_equal(body_trails_point_at(&trails, 7, 0), system.bodies[7].position_m);
```

For append test, assert counts and newest points for indexes `6` and `7` as well.

**Step 2: Run RED**

Run:

```bash
make build/tests/test_body_trails
```

Expected: FAIL until the eight-body factory exists and body capacity is 8.

**Step 3: Update app scene**

In `src/main.c`, replace:

```c
SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars();
```

with:

```c
SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos();
```

**Step 4: Run GREEN**

Run:

```bash
make build/tests/test_body_trails && build/tests/test_body_trails
make
```

Expected: PASS/build success.

---

## Task 7: Keep Phobos/Deimos visible in illustrative mode

**Objective:** Avoid tiny Martian moons being hidden inside Mars's large illustrative radius while preserving true simulation state.

**Files:**
- Modify: `tests/test_renderer.c`
- Modify: `src/render/renderer.c`
- Possibly modify: `src/render/renderer.h`
- Possibly modify: `src/sim/constants.h`

**Current renderer issue:** `renderer_body_radius()` currently scales all `BODY_KIND_MOON` radii by `SOLAR_ILLUSTRATIVE_PLANET_RADIUS * radius / EarthRadius`. That makes Phobos and Deimos nearly invisible. `renderer_body_position()` only expands Earth's Moon relative to Earth. Phobos and Deimos will render almost at Mars's center unless a Mars-relative illustrative offset policy is added.

**Step 1: Add renderer RED tests**

Add to `tests/test_renderer.c`:

```c
static void test_illustrative_martian_moons_are_visible_and_outside_mars(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos();
    Vec3d mars_position = renderer_body_position(&system, 5, RENDER_SCALE_ILLUSTRATIVE);
    Vec3d phobos_position = renderer_body_position(&system, 6, RENDER_SCALE_ILLUSTRATIVE);
    Vec3d deimos_position = renderer_body_position(&system, 7, RENDER_SCALE_ILLUSTRATIVE);
    double phobos_distance = vec3d_length(vec3d_sub(phobos_position, mars_position));
    double deimos_distance = vec3d_length(vec3d_sub(deimos_position, mars_position));
    double mars_radius = renderer_body_radius(&system.bodies[5], RENDER_SCALE_ILLUSTRATIVE);
    double phobos_radius = renderer_body_radius(&system.bodies[6], RENDER_SCALE_ILLUSTRATIVE);
    double deimos_radius = renderer_body_radius(&system.bodies[7], RENDER_SCALE_ILLUSTRATIVE);

    assert(phobos_radius > 0.0);
    assert(deimos_radius > 0.0);
    assert(phobos_radius < mars_radius);
    assert(deimos_radius < mars_radius);
    assert(mars_radius + phobos_radius < phobos_distance);
    assert(mars_radius + deimos_radius < deimos_distance);
}
```

Register in `main()`.

**Step 2: Run RED**

Run:

```bash
make build/tests/test_renderer && build/tests/test_renderer
```

Expected: FAIL because current illustrative positions for Phobos/Deimos remain too close to Mars and/or radii are too tiny to be useful.

**Step 3: Implement minimal renderer policy**

Prefer a generic helper inside `src/render/renderer.c` rather than duplicating per-body logic:

```c
static int moon_parent_index(const SolarSystem *system, size_t body_index)
{
    const Body *body = &system->bodies[body_index];
    const char *parent_name = NULL;

    if (body_named(body, "Moon")) {
        parent_name = "Earth";
    } else if (body_named(body, "Phobos") || body_named(body, "Deimos")) {
        parent_name = "Mars";
    }

    if (parent_name == NULL) {
        return -1;
    }

    for (size_t i = 0; i < system->body_count; ++i) {
        if (body_named(&system->bodies[i], parent_name)) {
            return (int)i;
        }
    }

    return -1;
}
```

For illustrative moon positions, convert both parent/body positions to render units, then ensure the visual distance is at least `parent_radius + satellite_radius + gap`. Do not change real-scale output.

Suggested constants in `src/render/renderer.c`:

```c
#define SOLAR_ILLUSTRATIVE_SATELLITE_GAP_UNITS 0.03
#define SOLAR_ILLUSTRATIVE_SMALL_MOON_RADIUS 0.012f
```

Update `renderer_body_radius()`:

```c
if (body->kind == BODY_KIND_MOON) {
    float radius = SOLAR_ILLUSTRATIVE_PLANET_RADIUS * (float)(body->radius_m / SOLAR_EARTH_RADIUS_M);
    return radius < SOLAR_ILLUSTRATIVE_SMALL_MOON_RADIUS ? SOLAR_ILLUSTRATIVE_SMALL_MOON_RADIUS : radius;
}
```

Keep Earth's Moon readable; this slightly raises only tiny moons like Phobos/Deimos.

Update `renderer_body_position()` to use the generic parent helper. If the moon is Earth's Moon, the existing `SOLAR_ILLUSTRATIVE_MOON_DISTANCE_FACTOR` policy can stay. For Phobos/Deimos, normalize the parent-relative render vector and scale it to at least the required visual distance.

Pseudo-code:

```c
Vec3d relative = vec3d_sub(position, parent_position);
double current_distance = vec3d_length(relative);
double required_distance = renderer_body_radius(parent, mode) + renderer_body_radius(body, mode) + SOLAR_ILLUSTRATIVE_SATELLITE_GAP_UNITS;
if (body_named(body, "Moon")) {
    Vec3d expanded = vec3d_scale(relative, SOLAR_ILLUSTRATIVE_MOON_DISTANCE_FACTOR);
    if (vec3d_length(expanded) > required_distance) return vec3d_add(parent_position, expanded);
}
if (current_distance > 0.0 && current_distance < required_distance) {
    return vec3d_add(parent_position, vec3d_scale(relative, required_distance / current_distance));
}
return position;
```

Also update `renderer_trail_point_position()` to apply the same parent-relative policy using trail points at the same index, so the Phobos/Deimos trails follow the visible moons.

**Step 4: Run GREEN**

Run:

```bash
make build/tests/test_renderer && build/tests/test_renderer
```

Expected: PASS.

---

## Task 8: Add explicit colors for Phobos and Deimos

**Objective:** Make the two Martian moons distinguishable in the raylib scene.

**Files:**
- Modify: `src/render/renderer.c`

**Implementation:**

In `body_render_color()` add before default:

```c
if (body->name != NULL && strcmp(body->name, "Phobos") == 0) {
    return BROWN;
}

if (body->name != NULL && strcmp(body->name, "Deimos") == 0) {
    return DARKBROWN;
}
```

If `DARKBROWN` is unavailable in the local raylib version, use `MAROON` or a custom `Color` literal.

**Verification:**

Run:

```bash
make
```

Expected: app builds.

---

## Task 9: Update overlay and README

**Objective:** Document Milestone 7 and make manual smoke checks obvious.

**Files:**
- Modify: `src/main.c`
- Modify: `README.md`

**Main overlay updates:**

Change milestone title to:

```c
DrawText("Solar System Simulator - Milestone 7: Phobos + Deimos", 20, 20, 20, RAYWHITE);
```

Change body count line to include all bodies:

```c
DrawText(TextFormat("Bodies: %zu (Sun + Mercury + Venus + Earth + Moon + Mars + Phobos + Deimos)", system.body_count), 20, 50, 18, RAYWHITE);
```

Add or replace one overlay line:

```c
DrawText("Phobos and Deimos start at Mars-relative periareion with vis-viva speed", 20, 325, 18, RAYWHITE);
```

Shift existing grid/trail overlay lines down if needed. Keep text on-screen; if the overlay grows too tall, shorten older lines rather than adding a full UI system.

**README updates:**

Update:

- Milestone header to `Milestone 7: Foundation + Sun + Mercury + Venus + Earth + Moon + Mars + Phobos + Deimos`.
- Current milestone paragraph to state Phobos and Deimos are Mars-relative moons.
- Current behavior list:
  - Renders exactly eight bodies.
  - Initializes Phobos/Deimos at Mars-relative periareion with tangential relative velocities.
  - Advances all non-fixed bodies with Newtonian gravity.
  - Trails include Phobos and Deimos and never extinguish during a run.
- Data table rows for Phobos and Deimos.
- Add a short `Martian moon orbital values` subsection with constants above.
- Controls: camera focus cycles through all eight bodies.
- Future increments: next body is likely asteroid belt representatives or Jupiter; do not leave Phobos/Deimos listed as future work.

**Verification:**

Run:

```bash
git diff --check
```

Expected: no whitespace errors.

---

## Task 10: Full verification and commit/push

**Objective:** Prove all ACs and push the completed iteration.

**Commands:**

```bash
make clean && make && make test
set +e
timeout 3s make run > /tmp/solar-phobos-deimos-run.log 2>&1
run_code=$?
set -e
if [ "$run_code" -ne 124 ] && [ "$run_code" -ne 0 ]; then
  cat /tmp/solar-phobos-deimos-run.log
  exit "$run_code"
fi
grep -E 'Initializing raylib|Device initialized successfully|PLATFORM: .*Initialized successfully' /tmp/solar-phobos-deimos-run.log
git diff --check
```

Additional scope checks:

```bash
# src/sim must stay raylib-independent
if grep -R "raylib\|Vector3\|Draw" -n src/sim; then
  echo "Unexpected render dependency in src/sim" >&2
  exit 1
fi

# inspect the final diff before commit
git diff --stat
git diff -- src/sim/constants.h src/sim/solar_system.h src/sim/solar_system.c tests/test_solar_system.c tests/test_body_trails.c tests/test_renderer.c src/render/renderer.c src/main.c README.md
```

Expected:

- All tests pass.
- Bounded smoke initializes raylib successfully.
- No whitespace errors.
- No raylib/render dependency appears under `src/sim/`.
- Diff is scoped to Phobos/Deimos simulation, app/render visibility, tests, and README.

Commit and push:

```bash
git add README.md src/main.c src/render/renderer.c src/render/renderer.h src/sim/constants.h src/sim/solar_system.c src/sim/solar_system.h tests/test_body_trails.c tests/test_renderer.c tests/test_solar_system.c
git commit -m "feat: add Phobos and Deimos milestone"
git push origin main
git status --short
git rev-list --left-right --count HEAD...origin/main
```

Expected final sync check: `0 0`.

---

## Risks and Tradeoffs

- **Tiny orbital periods:** Phobos orbits in about 7.65 hours. App-level fixed substeps are the minimal stabilization path; do not change global time scale or weaken physics tests unless evidence requires it.
- **Illustrative rendering:** Phobos and Deimos are physically tiny and close to Mars. Rendering needs parent-relative visual separation and a small visible moon floor, but this must stay in `src/render/` only.
- **No real ephemerides:** This iteration uses deterministic periareion initialization, not J2000/Horizons state vectors. That is consistent with current milestones.
- **Parent mapping:** Renderer can hardcode Moon->Earth and Phobos/Deimos->Mars for this milestone. A generalized parent field can wait until future multi-moon systems make it necessary.
- **Trail memory:** Trails intentionally never extinguish for this repo. Do not reintroduce fixed wrapping buffers.

---

## Ready-to-Execute Summary

Implement in this order:

1. RED constants tests, then constants.
2. RED body factory tests, then Phobos/Deimos factories and capacity.
3. RED eight-body scene test, then scene factory.
4. Parent-relative movement/orbit tests.
5. App physics substeps.
6. App scene + trail tests.
7. Renderer visibility/position/trail tests and implementation.
8. Colors, overlay, README.
9. Full verification.
10. Commit and push.
