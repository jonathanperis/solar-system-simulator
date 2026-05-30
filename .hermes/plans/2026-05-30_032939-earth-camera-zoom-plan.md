# Earth + Stable Camera Zoom Implementation Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task. Keep this plan local unless Jonathan explicitly asks to commit it.

**Goal:** Add Earth as the next physical body and fix the camera zoom behavior so excessive zoom-in never changes the default viewing angle or leaves the camera in a strange orientation for zooming back out.

**Architecture:** Preserve the physics-first split: `src/sim/` owns C-only double-precision SI simulation, `src/render/` owns raylib drawing policy, and `src/main.c` owns app state, camera controls, and overlays. Add Earth exactly as one new body; fix the camera by replacing reliance on raylib's opaque `UpdateCamera(..., CAMERA_ORBITAL)` with a tiny app-local orbit camera state that clamps distance while preserving yaw/pitch/default angle.

**Tech Stack:** C11, raylib, raymath only at render/app boundary, Makefile, C `assert` test binaries, headless raylib smoke via `PLATFORM=Memory`.

---

## Progress Snapshot

| Field | Status |
|---|---|
| Repo | `/opt/data/github/jonathanperis/solar-system-simulator` |
| Remote | `origin https://github.com/jonathanperis/solar-system-simulator.git` |
| Branch/ref inspected | `main` at `d78983d` |
| Working tree before plan | Clean/synced (`## main...origin/main`) |
| Prior milestones consulted | Mercury implementation, Venus implementation, `.hermes/plans/2026-05-30_030226-venus-view-modes-camera-focus.md` |
| Current implementation inspected | `src/main.c`, `src/sim/constants.h`, `src/sim/solar_system.*`, `src/render/renderer.c`, `tests/test_solar_system.c`, `README.md` |
| External evidence checked | Earth fact sheet values for mass, mean radius, semi-major axis, eccentricity, orbital period, perihelion/aphelion, orbital velocities |
| Implementation status | Not started; planning only |
| Validation status | Not applicable yet |

Overall planning progress: Earth and the camera zoom/angle fix are specified. Implementation should stop after adding Earth, fixing the zoom-angle behavior, updating docs/tests, running acceptance checks, and final review. Do not add the Moon or Mars in this iteration.

---

## Requirements and Non-Goals

### Requirements

- Add Earth as the fourth celestial body.
- Preserve C-only implementation.
- Preserve raylib-independent simulation under `src/sim/`.
- Keep internal state in double-precision SI units.
- Use real Earth mass and mean radius in simulation data.
- Initialize Earth with a physically meaningful heliocentric orbit using perihelion and vis-viva speed.
- Expand body storage deliberately to Sun + Mercury + Venus + Earth.
- Keep Sun fixed for this iteration.
- Keep Mercury and Venus behavior/tests intact.
- Keep illustrative and real-scale visualization modes intact.
- Camera focus cycling must include Sun, Mercury, Venus, and Earth.
- Fix zoom-in behavior: max zoom-in must not let raylib's camera helper change/flip/lose the default viewing angle.
- Zooming back out after max zoom-in must preserve the same orbit angle relationship to the focused body.
- Overlay must display current body count, view mode, camera focus, and camera zoom notes.
- README must document Earth scope, controls, camera zoom behavior, and the next planned body.
- Add tests for all non-windowing Earth simulation behavior and for pure camera math where feasible.

### Non-Goals

- Do not add the Moon, Mars, other bodies, atmosphere, rotation, axial tilt, seasons, textures, trails, labels, shadows, UI menus, or scene-file loading.
- Do not switch to JPL/Horizons epoch vectors.
- Do not implement barycentric Sun motion yet.
- Do not make screenshot or visual pixel tests part of automated checks.
- Do not create a generalized camera framework; implement the minimum app-local state required to keep zoom stable.

---

## Earth Source Data and Derived Values

Use existing simulator constants unless a task explicitly changes them:

- `SOLAR_G = 6.67430e-11`
- `SOLAR_SUN_MASS_KG = 1.98847e30`
- `SOLAR_AU_METERS = 149597870700.0`
- `SOLAR_DAY_SECONDS = 86400.0`

Planned Earth constants:

| Quantity | Value |
|---|---:|
| Mass | `5.9736e24 kg` |
| Mean radius | `6371000.0 m` |
| Semi-major axis | `149597887155.76578 m` (`1.00000011 AU`) |
| Eccentricity | `0.01671022` |
| Perihelion distance | `147098073549.85776 m` |
| Aphelion distance | `152097700761.6738 m` |
| Perihelion speed | `30287.085630725956 m/s` derived by vis-viva |
| Aphelion speed | `29291.514121573095 m/s` |
| Orbital period from current constants | `31557724.079989415 s` = `365.2514361109886 days` |
| Render perihelion at 1 AU = 10 units | `9.832898881618759` |
| Render aphelion at 1 AU = 10 units | `10.16710331838124` |

Source evidence: Earth fact-sheet style values list mass `5.9736 x 10^24 kg`, volumetric mean radius `6371.0 km`, semimajor axis near `149.60 x 10^6 km`, orbital eccentricity `0.0167`, perihelion `147.09 x 10^6 km`, aphelion `152.10 x 10^6 km`, and period about `365.256 days`. The plan uses J2000 `1.00000011 AU` and eccentricity `0.01671022` for deterministic constants.

### Initial Orbit Convention

- Keep Mercury and Venus initial states unchanged.
- Add Earth at perihelion.
- To avoid exact overlap with existing startup rays, place Earth in the XZ plane on the +Z side:
  - `position_m = { 0.0, 0.0, SOLAR_EARTH_PERIHELION_M }`
  - `velocity_mps = { SOLAR_EARTH_PERIHELION_SPEED_MPS, 0.0, 0.0 }`
- This is tangential to the Sun-Earth radius and preserves the same prograde orientation as Mercury/Venus when viewed from +Y.
- Tests should verify Earth acceleration points toward the Sun from the +Z position (`z < 0.0` acceleration).

---

## Camera Zoom Root-Cause Hypothesis

Current app uses:

```c
UpdateCamera(&camera, CAMERA_ORBITAL);
```

`CAMERA_ORBITAL` owns camera movement internally. When the user zooms too close to the target, raylib's helper can push the camera into an awkward orientation/angle around the target. Because the app only resets `camera.target`, not the orbit basis, the strange angle persists when zooming back out.

Planned fix:

- Remove `UpdateCamera(&camera, CAMERA_ORBITAL)` from the app loop.
- Keep app-local camera state:
  - `yaw_radians`
  - `pitch_radians`
  - `distance`
  - min/max zoom distances
- Apply mouse-wheel zoom by changing only `distance` and clamping it.
- Recompute `camera.position` every frame from target + yaw/pitch/distance.
- Keep yaw/pitch fixed at the default angle for this iteration. That directly satisfies the user's requirement that zoom should not alter the default angle, especially at max zoom-in.
- Continue updating `camera.target` from the focused body.

This is intentionally smaller and more deterministic than a general orbit/drag camera. A future milestone can add controlled yaw/pitch input if wanted.

---

## Task Plan

### Task 1: Add Earth constants and derived orbital macros

**Objective:** Centralize Earth physical/orbital values in SI units.

**Files:**
- Modify: `src/sim/constants.h`
- Test: `tests/test_solar_system.c`

**Step 1: Write failing tests**

Add to `tests/test_solar_system.c`:

```c
static void test_earth_constants_are_real_si_values(void)
{
    assert_close(SOLAR_EARTH_MASS_KG, 5.9736e24, 5.9736e24 * 1e-12);
    assert_close(SOLAR_EARTH_RADIUS_M, 6371000.0, 1e-6);
    assert_close(SOLAR_EARTH_SEMI_MAJOR_AXIS_M, 149597887155.76578, 1e-3);
    assert_close(SOLAR_EARTH_ECCENTRICITY, 0.01671022, 1e-12);
}

static void test_earth_perihelion_distance_is_derived_from_orbital_elements(void)
{
    double expected = SOLAR_EARTH_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_EARTH_ECCENTRICITY);

    assert_close(SOLAR_EARTH_PERIHELION_M, expected, 1e-3);
}

static void test_earth_perihelion_speed_matches_vis_viva(void)
{
    double expected = sqrt(SOLAR_G * SOLAR_SUN_MASS_KG *
        ((2.0 / SOLAR_EARTH_PERIHELION_M) - (1.0 / SOLAR_EARTH_SEMI_MAJOR_AXIS_M)));

    assert_close(SOLAR_EARTH_PERIHELION_SPEED_MPS, expected, 1e-6);
}
```

Add calls in `main()`.

**Step 2: Verify RED**

Run:

```bash
make test
```

Expected: FAIL because Earth constants are undefined.

**Step 3: Implement constants**

Add to `src/sim/constants.h` after Venus constants:

```c
#define SOLAR_EARTH_MASS_KG 5.9736e24
#define SOLAR_EARTH_RADIUS_M 6371000.0
#define SOLAR_EARTH_SEMI_MAJOR_AXIS_M 149597887155.76578
#define SOLAR_EARTH_ECCENTRICITY 0.01671022
#define SOLAR_EARTH_PERIHELION_M \
    (SOLAR_EARTH_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_EARTH_ECCENTRICITY))
#define SOLAR_EARTH_PERIHELION_SPEED_MPS \
    (sqrt(SOLAR_G * SOLAR_SUN_MASS_KG * \
        ((2.0 / SOLAR_EARTH_PERIHELION_M) - (1.0 / SOLAR_EARTH_SEMI_MAJOR_AXIS_M))))
```

**Step 4: Verify GREEN**

Run:

```bash
make test
```

Expected: PASS.

**Step 5: Commit**

```bash
git add src/sim/constants.h tests/test_solar_system.c
git commit -m "feat: add Earth physical constants"
```

---

### Task 2: Expand solar-system capacity to four bodies

**Objective:** Allow Sun + Mercury + Venus + Earth without heap allocation.

**Files:**
- Modify: `src/sim/solar_system.h`
- Test: `tests/test_solar_system.c`

**Step 1: Write failing test**

Replace the capacity test with:

```c
static void test_solar_system_capacity_supports_sun_mercury_venus_and_earth(void)
{
    assert(SOLAR_SYSTEM_BODY_CAPACITY == 4);
}
```

Update the call in `main()`.

**Step 2: Verify RED**

Run:

```bash
make test
```

Expected: FAIL because capacity is still `3`.

**Step 3: Implement capacity expansion**

In `src/sim/solar_system.h`:

```c
#define SOLAR_SYSTEM_BODY_CAPACITY 4
```

Do not change body counts in earlier factories.

**Step 4: Verify GREEN**

Run:

```bash
make test
```

Expected: PASS.

**Step 5: Commit**

```bash
git add src/sim/solar_system.h tests/test_solar_system.c
git commit -m "feat: expand solar system capacity for Earth"
```

---

### Task 3: Add Earth body initializer

**Objective:** Create a deterministic Earth body factory at perihelion.

**Files:**
- Modify: `src/sim/solar_system.h`
- Modify: `src/sim/solar_system.c`
- Test: `tests/test_solar_system.c`

**Step 1: Write failing test**

Add:

```c
static void test_earth_body_starts_at_perihelion_with_tangential_velocity(void)
{
    Body earth = solar_system_create_earth_at_perihelion();

    assert(strcmp(earth.name, "Earth") == 0);
    assert(earth.kind == BODY_KIND_PLANET);
    assert(!earth.fixed);
    assert_close(earth.mass_kg, SOLAR_EARTH_MASS_KG, SOLAR_EARTH_MASS_KG * 1e-12);
    assert_close(earth.radius_m, SOLAR_EARTH_RADIUS_M, 1e-6);
    assert_close(earth.position_m.x, 0.0, 1e-12);
    assert_close(earth.position_m.y, 0.0, 1e-12);
    assert_close(earth.position_m.z, SOLAR_EARTH_PERIHELION_M, 1e-3);
    assert_close(earth.velocity_mps.x, SOLAR_EARTH_PERIHELION_SPEED_MPS, 1e-6);
    assert_close(earth.velocity_mps.y, 0.0, 1e-12);
    assert_close(earth.velocity_mps.z, 0.0, 1e-12);
}
```

Add call in `main()`.

**Step 2: Verify RED**

Run:

```bash
make test
```

Expected: FAIL because `solar_system_create_earth_at_perihelion()` is undefined.

**Step 3: Implement helper**

In `src/sim/solar_system.h`:

```c
Body solar_system_create_earth_at_perihelion(void);
```

In `src/sim/solar_system.c` after Venus:

```c
Body solar_system_create_earth_at_perihelion(void)
{
    return body_create(
        "Earth",
        BODY_KIND_PLANET,
        SOLAR_EARTH_MASS_KG,
        SOLAR_EARTH_RADIUS_M,
        (Vec3d){0.0, 0.0, SOLAR_EARTH_PERIHELION_M},
        (Vec3d){SOLAR_EARTH_PERIHELION_SPEED_MPS, 0.0, 0.0},
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
git commit -m "feat: initialize Earth body state"
```

---

### Task 4: Add Sun + Mercury + Venus + Earth initializer

**Objective:** Create the default four-body scene while preserving previous factories.

**Files:**
- Modify: `src/sim/solar_system.h`
- Modify: `src/sim/solar_system.c`
- Test: `tests/test_solar_system.c`

**Step 1: Write failing test**

Add:

```c
static void test_sun_mercury_venus_earth_system_has_four_expected_bodies(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth();

    assert(system.body_count == 4);
    assert(strcmp(system.bodies[0].name, "Sun") == 0);
    assert(strcmp(system.bodies[1].name, "Mercury") == 0);
    assert(strcmp(system.bodies[2].name, "Venus") == 0);
    assert(strcmp(system.bodies[3].name, "Earth") == 0);
    assert(system.bodies[0].fixed);
    assert(!system.bodies[1].fixed);
    assert(!system.bodies[2].fixed);
    assert(!system.bodies[3].fixed);
    assert(system.bodies[3].kind == BODY_KIND_PLANET);
}
```

Add call in `main()`.

**Step 2: Verify RED**

Run:

```bash
make test
```

Expected: FAIL because `solar_system_create_sun_mercury_venus_earth()` is undefined.

**Step 3: Implement initializer**

In `src/sim/solar_system.h`:

```c
SolarSystem solar_system_create_sun_mercury_venus_earth(void);
```

In `src/sim/solar_system.c`:

```c
SolarSystem solar_system_create_sun_mercury_venus_earth(void)
{
    SolarSystem system = {
        .bodies = {
            create_sun(),
            solar_system_create_mercury_at_perihelion(),
            solar_system_create_venus_at_perihelion(),
            solar_system_create_earth_at_perihelion(),
        },
        .body_count = 4,
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
git commit -m "feat: create Sun Mercury Venus Earth system"
```

---

### Task 5: Prove Earth moves under Newtonian gravity

**Objective:** Add Earth physics regressions for acceleration direction, one-day movement, and rough orbital boundedness.

**Files:**
- Modify: `tests/test_solar_system.c`
- Possibly modify: `src/sim/physics.c` only if a real bug is exposed

**Step 1: Add gravity direction test**

```c
static void test_earth_acceleration_points_toward_sun_at_perihelion(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth();

    solar_system_step(&system, 0.0);

    assert_close(system.bodies[3].acceleration_mps2.x, 0.0, 1e-9);
    assert_close(system.bodies[3].acceleration_mps2.y, 0.0, 1e-18);
    assert(system.bodies[3].acceleration_mps2.z < 0.0);
}
```

Note: A tiny nonzero X contribution can appear from Mercury/Venus perturbations depending on exact placement. If this test fails only because X is physically nonzero, update the test to assert the Z component points toward the Sun and dominates `fabs(x)` rather than pretending the model is Sun-only.

**Step 2: Add one-day movement test**

```c
static void test_earth_moves_after_one_day_while_sun_stays_fixed(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth();
    Vec3d initial_earth_position = system.bodies[3].position_m;

    solar_system_step(&system, SOLAR_DAY_SECONDS);

    assert_close(system.bodies[0].position_m.x, 0.0, 1e-12);
    assert_close(system.bodies[0].position_m.y, 0.0, 1e-12);
    assert_close(system.bodies[0].position_m.z, 0.0, 1e-12);
    assert(system.bodies[3].position_m.x > initial_earth_position.x);
    assert(system.bodies[3].position_m.z < initial_earth_position.z);
}
```

**Step 3: Add rough orbit test**

```c
static void test_earth_roughly_returns_after_one_orbit(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth();
    Vec3d initial = system.bodies[3].position_m;
    const double dt_seconds = 24.0 * 60.0 * 60.0;
    const int steps = (int)((365.2514 * SOLAR_DAY_SECONDS) / dt_seconds);

    for (int i = 0; i < steps; ++i) {
        solar_system_step(&system, dt_seconds);
    }

    double distance_from_initial = vec3d_length(vec3d_sub(system.bodies[3].position_m, initial));
    assert(distance_from_initial < 0.08 * SOLAR_AU_METERS);
}
```

**Step 4: Verify**

Run:

```bash
make test
```

Expected: PASS. If a test fails, inspect the sign convention and multi-body perturbations before changing the test.

**Step 5: Commit**

```bash
git add tests/test_solar_system.c src/sim/physics.c
git commit -m "test: cover Earth orbital motion"
```

---

### Task 6: Add app-local stable orbit camera state

**Objective:** Replace raylib's opaque orbital camera helper with deterministic camera math that clamps zoom distance without changing the default angle.

**Files:**
- Modify: `src/main.c`
- Test: build/test/smoke/manual behavior note

**Step 1: Define a small camera state**

At file scope in `src/main.c`:

```c
#include <math.h>

typedef struct OrbitCameraState {
    float yaw_radians;
    float pitch_radians;
    float distance;
    float min_distance;
    float max_distance;
    float zoom_speed;
} OrbitCameraState;
```

**Step 2: Add pure camera helper functions**

```c
static float clamp_float(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static OrbitCameraState orbit_camera_default_state(void)
{
    return (OrbitCameraState){
        .yaw_radians = 0.0f,
        .pitch_radians = 0.418224f,
        .distance = 19.697716f,
        .min_distance = 4.0f,
        .max_distance = 80.0f,
        .zoom_speed = 2.0f,
    };
}

static Vector3 orbit_camera_position(Vector3 target, const OrbitCameraState *state)
{
    float cos_pitch = cosf(state->pitch_radians);
    return (Vector3){
        target.x + state->distance * sinf(state->yaw_radians) * cos_pitch,
        target.y + state->distance * sinf(state->pitch_radians),
        target.z + state->distance * cosf(state->yaw_radians) * cos_pitch,
    };
}

static void orbit_camera_apply_zoom(OrbitCameraState *state, float wheel_move)
{
    state->distance = clamp_float(
        state->distance - (wheel_move * state->zoom_speed),
        state->min_distance,
        state->max_distance
    );
}

static void apply_orbit_camera(Camera3D *camera, const OrbitCameraState *state, Vector3 target)
{
    camera->target = target;
    camera->position = orbit_camera_position(target, state);
}
```

The default state preserves the current visible angle derived from the old initial camera position `{0, 8, 18}` relative to origin.

**Step 3: Replace `UpdateCamera` usage**

In `main()` after camera initialization:

```c
OrbitCameraState orbit_camera = orbit_camera_default_state();
```

Inside the loop, replace:

```c
camera.target = body_camera_target(&system, focused_body_index);
UpdateCamera(&camera, CAMERA_ORBITAL);
solar_system_step(...);
camera.target = body_camera_target(&system, focused_body_index);
```

with:

```c
Vector3 camera_target = body_camera_target(&system, focused_body_index);
orbit_camera_apply_zoom(&orbit_camera, GetMouseWheelMove());
apply_orbit_camera(&camera, &orbit_camera, camera_target);
solar_system_step(&system, (double)GetFrameTime() * time_scale);
camera_target = body_camera_target(&system, focused_body_index);
apply_orbit_camera(&camera, &orbit_camera, camera_target);
```

**Step 4: Update overlay**

Add:

```c
DrawText(TextFormat("Camera zoom: %.1f units (wheel, clamped)", orbit_camera.distance), 20, 175, 18, RAYWHITE);
DrawText("Camera angle stays fixed while zooming", 20, 200, 18, RAYWHITE);
```

Shift subsequent overlay lines downward.

**Step 5: Verify**

Run:

```bash
make
make test
```

Expected: PASS. Then smoke:

```bash
set +e
timeout 3s make run > /tmp/solar-earth-camera-run.log 2>&1
run_code=$?
set -e
if [ "$run_code" -ne 124 ] && [ "$run_code" -ne 0 ]; then
  cat /tmp/solar-earth-camera-run.log
  exit "$run_code"
fi
grep -E 'Initializing raylib|Device initialized successfully|PLATFORM: .*Initialized successfully' /tmp/solar-earth-camera-run.log
```

Expected: raylib initializes successfully.

**Manual desktop check:** zoom in to minimum distance with mouse wheel and then zoom back out. The camera must keep the same viewing angle and not flip/pitch into a strange orientation.

**Step 6: Commit**

```bash
git add src/main.c
git commit -m "fix: keep camera angle stable while zooming"
```

---

### Task 7: Switch default app scene to Earth milestone

**Objective:** Launch Sun + Mercury + Venus + Earth by default and include Earth in camera focus cycling.

**Files:**
- Modify: `src/main.c`
- Test: build/test/smoke

**Step 1: Update initializer**

Change:

```c
SolarSystem system = solar_system_create_sun_mercury_venus();
```

to:

```c
SolarSystem system = solar_system_create_sun_mercury_venus_earth();
```

Focus cycling already uses `system.body_count`, so Earth should be included automatically.

**Step 2: Update overlay title/body text**

```c
DrawText("Solar System Simulator - Milestone 4: Earth", 20, 20, 20, RAYWHITE);
DrawText(TextFormat("Bodies: %zu (Sun + Mercury + Venus + Earth)", system.body_count), 20, 50, 18, RAYWHITE);
DrawText("Mercury, Venus, and Earth start at perihelion with vis-viva tangential speeds", ...);
```

**Step 3: Verify**

Run:

```bash
make
make test
```

Expected: PASS.

**Step 4: Commit**

```bash
git add src/main.c
git commit -m "feat: launch Earth simulation"
```

---

### Task 8: Render Earth distinctly while keeping renderer generic

**Objective:** Make Earth visually distinct without body-specific draw calls in `main.c`.

**Files:**
- Modify: `src/render/renderer.c`
- Test: build/smoke

**Step 1: Update color helper**

In `body_render_color()`:

```c
if (body->name != NULL && strcmp(body->name, "Earth") == 0) {
    return BLUE;
}
```

Place after Venus check.

**Step 2: Verify**

Run:

```bash
make
make test
```

Expected: PASS.

**Step 3: Commit**

```bash
git add src/render/renderer.c
git commit -m "feat: render Earth distinctly"
```

---

### Task 9: Document Earth and stable camera zoom

**Objective:** Update README to describe the new body, milestone scope, controls, and zoom behavior accurately.

**Files:**
- Modify: `README.md`

**Step 1: Update milestone section**

Rename to:

```markdown
## Milestone 4: Foundation + Sun + Mercury + Venus + Earth
```

State current behavior:

- renders exactly Sun, Mercury, Venus, and Earth;
- Mercury, Venus, and Earth start at perihelion using vis-viva tangential speeds;
- Sun remains fixed;
- `V` toggles illustrative vs real-scale visualization;
- `Tab` or `C` cycles camera focus across all bodies;
- mouse wheel zoom is clamped and preserves camera angle.

**Step 2: Add Earth data**

Extend the body table:

```markdown
| Earth | `5.9736e24 kg` | `6371000 m` | perihelion position and tangential speed |
```

Include:

- semi-major axis: `149597887155.76578 m`
- eccentricity: `0.01671022`
- perihelion distance: `147098073549.85776 m`
- perihelion speed: `30287.085630725956 m/s`

**Step 3: Update controls section**

```markdown
- Mouse wheel: zoom camera in/out around the focused body.
  - Zoom distance is clamped.
  - The viewing angle remains fixed so max zoom-in does not flip or corrupt the camera orientation.
```

**Step 4: Update roadmap**

Remove Earth from next planned iterations and make Moon next:

```markdown
1. Moon
2. Mars
3. asteroid belt representatives / major asteroids
...
```

**Step 5: Verify README accuracy**

Check README does not claim:

- Moon exists;
- accurate ephemerides;
- barycentric Sun motion;
- camera drag/orbit controls beyond the clamped fixed-angle zoom;
- real-scale mode makes planets visually easy to see.

**Step 6: Commit**

```bash
git add README.md
git commit -m "docs: document Earth iteration"
```

---

### Task 10: Final acceptance checks and review

**Objective:** Confirm the Earth + camera zoom iteration meets all criteria before final push/report.

**Files:**
- No intended source changes unless review finds issues.

**Step 1: Run full checks**

```bash
make clean
make
make test
set +e
timeout 3s make run > /tmp/solar-earth-run.log 2>&1
run_code=$?
set -e
if [ "$run_code" -ne 124 ] && [ "$run_code" -ne 0 ]; then
  cat /tmp/solar-earth-run.log
  exit "$run_code"
fi
grep -E 'Initializing raylib|Device initialized successfully|PLATFORM: .*Initialized successfully' /tmp/solar-earth-run.log
git diff --check
```

Expected:

- `make` succeeds.
- `make test` succeeds.
- bounded `make run` initializes raylib successfully.
- `git diff --check` has no whitespace errors.

**Step 2: Dispatch final review**

Use two reviewers:

- Spec compliance reviewer checks every requirement and acceptance criterion in this plan.
- Code quality reviewer checks simplicity, C style, test coverage, no scope creep, render/sim isolation, and camera math.

**Step 3: Fix review issues**

If review finds blockers, fix them and rerun full checks.

**Step 4: Final push**

```bash
git status --short
git push origin main
git rev-list --left-right --count HEAD...origin/main
```

Expected after push: `0 0`.

---

## Acceptance Criteria

- `make` builds `build/solar-system-simulator`.
- `make test` runs all C test binaries and passes.
- Bounded `make run` initializes raylib successfully; on a desktop, `make run` opens the window.
- The default scene contains exactly four celestial bodies: Sun, Mercury, Venus, and Earth.
- Sun remains fixed at the origin.
- Mercury and Venus behavior remain intact.
- Earth uses real mass and mean radius in simulation data.
- Earth initial position uses perihelion distance derived from semi-major axis and eccentricity.
- Earth initial velocity uses tangential perihelion speed derived from the vis-viva equation.
- Earth is non-fixed and moves under Newtonian gravity.
- Tests prove Earth acceleration points toward the Sun at perihelion.
- Tests prove Earth moves after stepping and remains approximately bound/periodic over one orbit.
- Rendering scale remains isolated from simulation units.
- Existing illustrative and real-scale visualization modes still work.
- Earth is visible/distinct in illustrative mode.
- Real-scale mode keeps physical radius scaling and no radius clamp.
- Camera focus cycles across every simulated body, including Earth.
- Mouse-wheel zoom is clamped to a minimum/maximum distance.
- Max zoom-in does not change/flip/corrupt the camera's default viewing angle.
- Zooming back out after max zoom-in preserves the same viewing angle.
- Overlay displays current body count, view mode, camera focus target, elapsed time, camera zoom, and physics/render scale notes.
- README documents Earth scope, assumptions, commands, view modes, camera controls/zoom behavior, and next planned body iteration.

---

## Risks and Mitigations

| Risk | Mitigation |
|---|---|
| Camera fix removes automatic orbital rotation | Accept for this milestone because the user specifically prioritized stable default angle during zoom; future controlled orbit input can be added deliberately |
| Camera position jumps when first applying custom math | Use yaw/pitch/distance derived from current initial `{0, 8, 18}` camera vector |
| Earth perturbation tests assume Sun-only gravity | Write assertions that allow small multi-body perturbations while requiring the dominant component points toward the Sun |
| Real-scale mode makes bodies hard to see | Keep illustrative mode default and document real-scale invisibility as expected |
| Capacity exact-value test breaks next iteration | Expected; each body iteration deliberately updates capacity |
| Scope creep into Moon/rotation/tilt | Stop after Earth and camera zoom fix |
| New camera helpers in `main.c` grow too much | Keep only fixed-angle zoom and focus targeting; no UI framework or drag/orbit controls |

---

## Open Questions for Later Iterations

These do not block Earth:

1. Should Moon be modeled relative to Earth first, or still heliocentrically until barycentric support exists?
2. Should camera yaw/pitch controls be added later as explicit keys/mouse drag with pitch clamps?
3. Should tests start tracking energy/angular-momentum drift once Earth/Moon exists?
4. Should render colors/styles move from string checks to a body style table as body count grows?
5. Should initial states switch from perihelion conventions to a common epoch before adding Moon?

---

## Implementation Handoff

Plan complete for Earth + stable camera zoom. Execute with strict TDD for simulation changes, root-cause discipline for camera behavior, two-stage review before final push, and full acceptance checks. Stop after Earth and the camera zoom-angle fix; do not add Moon or Mars in this iteration.
