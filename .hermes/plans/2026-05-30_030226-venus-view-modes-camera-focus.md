# Venus + View Modes + Camera Focus Implementation Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task. Keep this plan local unless Jonathan explicitly asks to commit it.

**Goal:** Add Venus as the third celestial body and introduce two visualization modes plus camera focus controls so the simulator can inspect Sun, Mercury, and Venus without locking the camera target to the Sun.

**Architecture:** Preserve the existing physics-first C/raylib split. Extend `src/sim/` with Venus physical/orbital state in double-precision SI units and keep raylib-free tests for constants, initialization, gravity, and orbit plausibility. Add a small render/app presentation layer for visualization modes and camera focus; these modes must never mutate simulation positions, masses, radii, or velocities.

**Tech Stack:** C11, raylib, raymath only at render/app boundary, Makefile, C test binaries using `assert`/return codes.

---

## Progress Snapshot

| Field | Status |
|---|---|
| Repo | `/opt/data/github/jonathanperis/solar-system-simulator` |
| Remote | `origin https://github.com/jonathanperis/solar-system-simulator.git` |
| Branch/ref inspected | `main` at `243797675390278f1daf5967eed0b3f4368c6274` |
| Working tree before plan | Clean/synced (`git status -sb` showed `## main...origin/main`) |
| Prior plans consulted | `.hermes/plans/2026-05-30_023343-mercury-iteration.md` |
| Current implementation inspected | `src/sim/constants.h`, `src/sim/solar_system.*`, `src/main.c`, `src/render/renderer.c`, `tests/test_solar_system.c`, `README.md` |
| External evidence checked | NASA Venus facts page for distance/size/orbit period; Venus fact-sheet style values for mass, radius, semi-major axis, eccentricity |
| Implementation status | Not started; planning only |
| Validation status | Not applicable yet |

Overall planning progress: Venus plus the requested visualization/camera controls are specified. Implementation should stop after adding Venus, two view modes, focusable camera targets for every body, README updates, tests, smoke checks, and final review. Do not add Earth or any other body in this iteration.

---

## Requirements and Non-Goals

### Requirements

- Add Venus as the third celestial body.
- Preserve C-only implementation.
- Preserve raylib-independent simulation under `src/sim/`.
- Keep internal state in double-precision SI units.
- Use real physical Venus mass and radius in simulation data.
- Initialize Venus with a physically meaningful heliocentric orbit, not a hand-tuned circular animation.
- Expand body storage deliberately to Sun + Mercury + Venus.
- Keep Sun fixed for this iteration.
- Keep Mercury behavior and tests intact.
- Add two visualization modes:
  1. **Illustrative mode**: practical default view where orbital positions use AU-scale rendering and body radii are visibly clamped/enlarged, like the current milestone.
  2. **Real-scale mode**: positions and radii both use the same physical render scale so users can see how tiny the bodies are relative to orbital distances. This mode may make planets nearly invisible; that is expected and should be documented in the overlay/README.
- Add camera focus controls so the camera can orbit/follow any body, not only the Sun/origin.
- Expose the current view mode and camera focus target in the overlay.
- Keep view mode and camera focus as presentation/app state only; no simulation data mutation.
- Add tests that validate the non-windowing parts of the feature.
- Run final acceptance checks before committing.

### Non-Goals

- Do not add Earth or other bodies.
- Do not add moons, Venus atmosphere, rotation, axial tilt, textures, shaders, trails, labels, or UI menus.
- Do not add dynamic allocation or scene-file loading.
- Do not implement barycentric Sun motion yet.
- Do not implement JPL/Horizons epoch state vectors.
- Do not add relativistic corrections or multi-body ephemeris accuracy claims.
- Do not make screenshots or visual pixel tests part of the automated test suite.

---

## Venus Source Data and Derived Values

Use the simulator's existing constants unless a prior task explicitly changes them:

- `SOLAR_G = 6.67430e-11`
- `SOLAR_SUN_MASS_KG = 1.98847e30`
- `SOLAR_AU_METERS = 149597870700.0`
- `SOLAR_DAY_SECONDS = 86400.0`

Planned Venus constants:

| Quantity | Value |
|---|---:|
| Mass | `4.8675e24 kg` |
| Mean radius | `6051800.0 m` |
| Semi-major axis | `108208000000.0 m` |
| Eccentricity | `0.006772` |
| Perihelion distance | `107475215424.0 m` |
| Aphelion distance | `108940784576.0 m` |
| Perihelion speed | `35259.30808092215 m/s` derived by vis-viva |
| Aphelion speed | `34784.96824166559 m/s` |
| Orbital period from current constants | `19413620.69378756 s` = `224.69468395587452 days` |
| Current render perihelion at 1 AU = 10 units | `7.184274409862974` |
| Current render aphelion at 1 AU = 10 units | `7.282241656665505` |

NASA Venus facts page supports the broad facts used in user-facing docs: Venus is second from the Sun, average distance is about 108 million km / 0.72 AU, diameter is about 12,104 km, and orbital period is about 225 Earth days.

### Initial Orbit Convention

- Keep Mercury's existing initial state unchanged.
- Add Venus at perihelion.
- To avoid exact same ray direction as Mercury at startup, place Venus in the same XZ plane but opposite the +X side:
  - `position_m = { -SOLAR_VENUS_PERIHELION_M, 0.0, 0.0 }`
  - `velocity_mps = { 0.0, 0.0, -SOLAR_VENUS_PERIHELION_SPEED_MPS }`
- This gives Venus tangential motion around the Sun in the same prograde orientation as Mercury when viewed from +Y, while keeping bodies visually separated at launch.
- Tests should verify acceleration points toward the Sun from Venus' initial position (`x > 0.0` acceleration because Venus starts at negative X).

---

## Design Notes

1. **Increase capacity only as far as needed.**
   - Change `SOLAR_SYSTEM_BODY_CAPACITY` from `2` to `3`.
   - Keep fixed array storage; no heap allocation yet.

2. **Prefer explicit scene factory naming.**
   - Keep `solar_system_create_sun_only()` for regression.
   - Keep `solar_system_create_sun_mercury()` for Mercury milestone regression.
   - Add `solar_system_create_sun_mercury_venus()` for the default app scene.

3. **Extract small body factories as needed.**
   - Existing `solar_system_create_mercury_at_perihelion()` is public.
   - Add `solar_system_create_venus_at_perihelion()`.
   - Keep `create_sun()` static unless tests need a public Sun factory.

4. **View modes belong to rendering/app code, not simulation.**
   - Add a render mode enum under `src/render/` or a small app-level enum in `src/main.c`.
   - Prefer renderer API taking an explicit mode, for example:
     ```c
     typedef enum RenderScaleMode {
         RENDER_SCALE_ILLUSTRATIVE,
         RENDER_SCALE_REAL
     } RenderScaleMode;

     void renderer_draw_solar_system(const SolarSystem *system, RenderScaleMode mode);
     Vector3 renderer_body_position(const Body *body);
     ```
   - `RENDER_SCALE_ILLUSTRATIVE`: convert positions with `meters_vec_to_render_vec3d()` and clamp radii per body class (`SOLAR_MIN_VISIBLE_BODY_RADIUS`, `SOLAR_MIN_VISIBLE_PLANET_RADIUS`).
   - `RENDER_SCALE_REAL`: convert positions and radii with the same `meters_to_render_units()` scale and do not clamp radii upward. A tiny optional wire marker can be debated later, but do not add it in this iteration unless launch smoke becomes visually unusable.

5. **Camera focus should target body positions in render space.**
   - Maintain `size_t focused_body_index` in `src/main.c`.
   - Each frame, set `camera.target` to the selected body's current rendered position.
   - Keep camera orbital behavior but re-center around the selected body.
   - Add key controls:
     - `TAB` or `C`: cycle focus to next body.
     - `V`: toggle visualization mode.
   - If raylib's `UpdateCamera(&camera, CAMERA_ORBITAL)` fights manual `camera.target`, set `camera.target` after `UpdateCamera`, or replace with a minimal manual orbit later. Start with the smallest change and verify by smoke/build.

6. **Testing strategy.**
   - Tests should cover all simulation state and non-windowing presentation helpers that can be separated from raylib windowing.
   - Do not open a raylib window in tests.
   - If renderer helper functions are static/private and hard to test, prefer testing through build and smoke unless a clean header-level helper is genuinely useful.

---

## Task Plan

### Task 1: Add Venus constants and derived orbital macros

**Objective:** Centralize Venus physical/orbital values in SI units.

**Files:**
- Modify: `src/sim/constants.h`
- Test: `tests/test_solar_system.c`

**Step 1: Write failing tests**

Add these tests to `tests/test_solar_system.c`:

```c
static void test_venus_constants_are_real_si_values(void)
{
    assert_close(SOLAR_VENUS_MASS_KG, 4.8675e24, 4.8675e24 * 1e-12);
    assert_close(SOLAR_VENUS_RADIUS_M, 6051800.0, 1e-6);
    assert_close(SOLAR_VENUS_SEMI_MAJOR_AXIS_M, 108208000000.0, 1e-3);
    assert_close(SOLAR_VENUS_ECCENTRICITY, 0.006772, 1e-12);
}

static void test_venus_perihelion_distance_is_derived_from_orbital_elements(void)
{
    double expected = SOLAR_VENUS_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_VENUS_ECCENTRICITY);

    assert_close(SOLAR_VENUS_PERIHELION_M, expected, 1e-3);
}

static void test_venus_perihelion_speed_matches_vis_viva(void)
{
    double expected = sqrt(SOLAR_G * SOLAR_SUN_MASS_KG *
        ((2.0 / SOLAR_VENUS_PERIHELION_M) - (1.0 / SOLAR_VENUS_SEMI_MAJOR_AXIS_M)));

    assert_close(SOLAR_VENUS_PERIHELION_SPEED_MPS, expected, 1e-6);
}
```

Add calls in `main()`.

**Step 2: Verify RED**

Run:

```bash
make test
```

Expected: FAIL because Venus constants are undefined.

**Step 3: Implement constants**

Add to `src/sim/constants.h` after Mercury constants:

```c
#define SOLAR_VENUS_MASS_KG 4.8675e24
#define SOLAR_VENUS_RADIUS_M 6051800.0
#define SOLAR_VENUS_SEMI_MAJOR_AXIS_M 108208000000.0
#define SOLAR_VENUS_ECCENTRICITY 0.006772
#define SOLAR_VENUS_PERIHELION_M \
    (SOLAR_VENUS_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_VENUS_ECCENTRICITY))
#define SOLAR_VENUS_PERIHELION_SPEED_MPS \
    (sqrt(SOLAR_G * SOLAR_SUN_MASS_KG * \
        ((2.0 / SOLAR_VENUS_PERIHELION_M) - (1.0 / SOLAR_VENUS_SEMI_MAJOR_AXIS_M))))
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
git commit -m "feat: add Venus physical constants"
```

---

### Task 2: Expand solar-system capacity to three bodies

**Objective:** Allow the simulation to store Sun + Mercury + Venus without heap allocation.

**Files:**
- Modify: `src/sim/solar_system.h`
- Test: `tests/test_solar_system.c`

**Step 1: Write failing test**

Replace the capacity test with:

```c
static void test_solar_system_capacity_supports_sun_mercury_and_venus(void)
{
    assert(SOLAR_SYSTEM_BODY_CAPACITY == 3);
}
```

Update the call in `main()`.

**Step 2: Verify RED**

Run:

```bash
make test
```

Expected: FAIL because capacity is still `2`.

**Step 3: Implement minimal capacity expansion**

In `src/sim/solar_system.h`:

```c
#define SOLAR_SYSTEM_BODY_CAPACITY 3
```

Do not change existing body counts in Sun-only or Sun+Mercury factories.

**Step 4: Verify GREEN**

Run:

```bash
make test
```

Expected: PASS.

**Step 5: Commit**

```bash
git add src/sim/solar_system.h tests/test_solar_system.c
git commit -m "feat: expand solar system capacity for Venus"
```

---

### Task 3: Add Venus body initializer

**Objective:** Create a reusable Venus body factory with deterministic perihelion state.

**Files:**
- Modify: `src/sim/solar_system.h`
- Modify: `src/sim/solar_system.c`
- Test: `tests/test_solar_system.c`

**Step 1: Write failing test**

Add:

```c
static void test_venus_body_starts_at_perihelion_with_tangential_velocity(void)
{
    Body venus = solar_system_create_venus_at_perihelion();

    assert(strcmp(venus.name, "Venus") == 0);
    assert(venus.kind == BODY_KIND_PLANET);
    assert(!venus.fixed);
    assert_close(venus.mass_kg, SOLAR_VENUS_MASS_KG, SOLAR_VENUS_MASS_KG * 1e-12);
    assert_close(venus.radius_m, SOLAR_VENUS_RADIUS_M, 1e-6);
    assert_close(venus.position_m.x, -SOLAR_VENUS_PERIHELION_M, 1e-3);
    assert_close(venus.position_m.y, 0.0, 1e-12);
    assert_close(venus.position_m.z, 0.0, 1e-12);
    assert_close(venus.velocity_mps.x, 0.0, 1e-12);
    assert_close(venus.velocity_mps.y, 0.0, 1e-12);
    assert_close(venus.velocity_mps.z, -SOLAR_VENUS_PERIHELION_SPEED_MPS, 1e-6);
}
```

Add call in `main()`.

**Step 2: Verify RED**

Run:

```bash
make test
```

Expected: FAIL because `solar_system_create_venus_at_perihelion()` is undefined.

**Step 3: Implement helper**

In `src/sim/solar_system.h`:

```c
Body solar_system_create_venus_at_perihelion(void);
```

In `src/sim/solar_system.c`:

```c
Body solar_system_create_venus_at_perihelion(void)
{
    return body_create(
        "Venus",
        BODY_KIND_PLANET,
        SOLAR_VENUS_MASS_KG,
        SOLAR_VENUS_RADIUS_M,
        (Vec3d){-SOLAR_VENUS_PERIHELION_M, 0.0, 0.0},
        (Vec3d){0.0, 0.0, -SOLAR_VENUS_PERIHELION_SPEED_MPS},
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
git commit -m "feat: initialize Venus body state"
```

---

### Task 4: Add Sun + Mercury + Venus solar-system initializer

**Objective:** Create the default three-body simulation state while keeping previous milestone factories.

**Files:**
- Modify: `src/sim/solar_system.h`
- Modify: `src/sim/solar_system.c`
- Test: `tests/test_solar_system.c`

**Step 1: Write failing test**

Add:

```c
static void test_sun_mercury_venus_system_has_three_expected_bodies(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus();

    assert(system.body_count == 3);
    assert(strcmp(system.bodies[0].name, "Sun") == 0);
    assert(strcmp(system.bodies[1].name, "Mercury") == 0);
    assert(strcmp(system.bodies[2].name, "Venus") == 0);
    assert(system.bodies[0].fixed);
    assert(!system.bodies[1].fixed);
    assert(!system.bodies[2].fixed);
    assert(system.bodies[2].kind == BODY_KIND_PLANET);
}
```

Add call in `main()`.

**Step 2: Verify RED**

Run:

```bash
make test
```

Expected: FAIL because `solar_system_create_sun_mercury_venus()` is undefined.

**Step 3: Implement initializer**

In `src/sim/solar_system.h`:

```c
SolarSystem solar_system_create_sun_mercury_venus(void);
```

In `src/sim/solar_system.c`:

```c
SolarSystem solar_system_create_sun_mercury_venus(void)
{
    SolarSystem system = {
        .bodies = {
            create_sun(),
            solar_system_create_mercury_at_perihelion(),
            solar_system_create_venus_at_perihelion(),
        },
        .body_count = 3,
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
git commit -m "feat: create Sun Mercury Venus system"
```

---

### Task 5: Prove Venus moves under Sun gravity

**Objective:** Add physics regression tests showing Venus accelerates toward the Sun, moves, and stays roughly bound/periodic.

**Files:**
- Modify: `tests/test_solar_system.c`
- Possibly modify: `src/sim/physics.c` only if a real bug is exposed

**Step 1: Add gravity direction test**

```c
static void test_venus_acceleration_points_toward_sun_at_perihelion(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus();

    solar_system_step(&system, 0.0);

    assert(system.bodies[2].acceleration_mps2.x > 0.0);
    assert_close(system.bodies[2].acceleration_mps2.y, 0.0, 1e-18);
    assert_close(system.bodies[2].acceleration_mps2.z, 0.0, 1e-18);
}
```

**Step 2: Add one-day movement test**

```c
static void test_venus_moves_after_one_day_while_sun_stays_fixed(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus();
    Vec3d initial_venus_position = system.bodies[2].position_m;

    solar_system_step(&system, SOLAR_DAY_SECONDS);

    assert_close(system.bodies[0].position_m.x, 0.0, 1e-12);
    assert_close(system.bodies[0].position_m.y, 0.0, 1e-12);
    assert_close(system.bodies[0].position_m.z, 0.0, 1e-12);
    assert(system.bodies[2].position_m.x > initial_venus_position.x);
    assert(system.bodies[2].position_m.z < initial_venus_position.z);
}
```

**Step 3: Add rough orbit test**

```c
static void test_venus_roughly_returns_after_one_orbit(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus();
    Vec3d initial = system.bodies[2].position_m;
    const double dt_seconds = 12.0 * 60.0 * 60.0;
    const int steps = (int)((224.6947 * SOLAR_DAY_SECONDS) / dt_seconds);

    for (int i = 0; i < steps; ++i) {
        solar_system_step(&system, dt_seconds);
    }

    double distance_from_initial = vec3d_length(vec3d_sub(system.bodies[2].position_m, initial));
    assert(distance_from_initial < 0.08 * SOLAR_AU_METERS);
}
```

**Step 4: Verify**

Run:

```bash
make test
```

Expected: PASS. If a new test fails, inspect sign convention, initializer state, and integrator behavior before changing the test.

**Step 5: Commit**

```bash
git add tests/test_solar_system.c src/sim/physics.c
git commit -m "test: cover Venus orbital motion"
```

---

### Task 6: Add render scale mode API

**Objective:** Make renderer support both illustrative and real-scale visualization modes without changing simulation data.

**Files:**
- Modify: `src/render/renderer.h`
- Modify: `src/render/renderer.c`
- Modify: `src/main.c` only as needed for compilation after signature change
- Test: build/test/smoke; no window-opening unit test required

**Step 1: Add enum to renderer header**

In `src/render/renderer.h`, add:

```c
typedef enum RenderScaleMode {
    RENDER_SCALE_ILLUSTRATIVE,
    RENDER_SCALE_REAL
} RenderScaleMode;

const char *renderer_scale_mode_label(RenderScaleMode mode);
void renderer_draw_solar_system(const SolarSystem *system, RenderScaleMode mode);
```

Update existing function declaration accordingly.

**Step 2: Implement label helper**

In `src/render/renderer.c`:

```c
const char *renderer_scale_mode_label(RenderScaleMode mode)
{
    switch (mode) {
        case RENDER_SCALE_REAL:
            return "Real scale";
        case RENDER_SCALE_ILLUSTRATIVE:
        default:
            return "Illustrative";
    }
}
```

**Step 3: Split radius policy by mode**

Keep positions physically scaled in both modes:

```c
static float body_render_radius(const Body *body, RenderScaleMode mode)
{
    float scaled_radius = meters_to_render_units(body->radius_m);

    if (mode == RENDER_SCALE_REAL) {
        return scaled_radius;
    }

    float min_radius = body_min_visible_radius(body);
    return scaled_radius < min_radius ? min_radius : scaled_radius;
}
```

Update call sites.

**Step 4: Update renderer function signature**

```c
void renderer_draw_solar_system(const SolarSystem *system, RenderScaleMode mode)
```

Use `mode` when computing radius.

**Step 5: Temporarily update `src/main.c`**

Until toggle is added:

```c
renderer_draw_solar_system(&system, RENDER_SCALE_ILLUSTRATIVE);
```

**Step 6: Verify**

Run:

```bash
make
make test
```

Expected: build/tests pass.

**Step 7: Commit**

```bash
git add src/render/renderer.h src/render/renderer.c src/main.c
git commit -m "feat: add render scale modes"
```

---

### Task 7: Add camera focus helper functions in app code

**Objective:** Keep camera target selection simple and deterministic before wiring keyboard controls.

**Files:**
- Modify: `src/main.c`
- Test: build/test/smoke

**Step 1: Add helper functions**

At file scope in `src/main.c`:

```c
static size_t next_body_index(size_t current, const SolarSystem *system)
{
    if (system->body_count == 0) {
        return 0;
    }

    return (current + 1) % system->body_count;
}

static Vector3 body_camera_target(const SolarSystem *system, size_t body_index)
{
    if (system->body_count == 0 || body_index >= system->body_count) {
        return (Vector3){0.0f, 0.0f, 0.0f};
    }

    Vec3d render_position = meters_vec_to_render_vec3d(system->bodies[body_index].position_m);
    return (Vector3){(float)render_position.x, (float)render_position.y, (float)render_position.z};
}
```

Do not add a new dependency from `src/sim/` to raylib; this helper remains in `src/main.c`.

**Step 2: Add focus state**

After `SolarSystem system = ...`:

```c
size_t focused_body_index = 0;
```

**Step 3: Set target each frame**

After stepping simulation and before drawing:

```c
camera.target = body_camera_target(&system, focused_body_index);
```

If `UpdateCamera(&camera, CAMERA_ORBITAL)` overwrites target, set `camera.target` again immediately after `UpdateCamera()`.

**Step 4: Verify**

Run:

```bash
make
make test
```

Expected: build/tests pass.

**Step 5: Commit**

```bash
git add src/main.c
git commit -m "feat: target camera at selected body"
```

---

### Task 8: Wire keyboard controls for view mode and body focus

**Objective:** Allow users to switch visualization mode and camera focus during runtime.

**Files:**
- Modify: `src/main.c`
- Test: build/test/smoke

**Step 1: Add view mode state**

After focus state:

```c
RenderScaleMode render_mode = RENDER_SCALE_ILLUSTRATIVE;
```

**Step 2: Handle key input**

Inside the loop before `UpdateCamera()`:

```c
if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_C)) {
    focused_body_index = next_body_index(focused_body_index, &system);
}

if (IsKeyPressed(KEY_V)) {
    render_mode = render_mode == RENDER_SCALE_ILLUSTRATIVE ? RENDER_SCALE_REAL : RENDER_SCALE_ILLUSTRATIVE;
}
```

**Step 3: Pass render mode to renderer**

```c
renderer_draw_solar_system(&system, render_mode);
```

**Step 4: Update overlay**

Add text similar to:

```c
const Body *focused_body = &system.bodies[focused_body_index];
DrawText(TextFormat("View: %s (V to toggle)", renderer_scale_mode_label(render_mode)), 20, 150, 18, RAYWHITE);
DrawText(TextFormat("Camera focus: %s (Tab/C to cycle)", focused_body->name), 20, 175, 18, RAYWHITE);
```

Update existing overlay y-positions to avoid overlap.

**Step 5: Verify**

Run:

```bash
make
make test
```

Expected: build/tests pass.

**Step 6: Commit**

```bash
git add src/main.c src/render/renderer.h src/render/renderer.c
git commit -m "feat: toggle view scale and camera focus"
```

---

### Task 9: Render Venus distinctly while keeping renderer generic

**Objective:** Make Venus visibly distinct from Mercury without hardcoded draw calls in `main.c`.

**Files:**
- Modify: `src/render/renderer.c`
- Test: build/smoke

**Step 1: Update color helper**

Keep renderer generic over bodies, but extend `body_render_color()`:

```c
if (body->name != NULL && strcmp(body->name, "Mercury") == 0) {
    return GRAY;
}

if (body->name != NULL && strcmp(body->name, "Venus") == 0) {
    return BEIGE;
}
```

Alternative acceptable if `BEIGE` is unavailable in the installed raylib version: use `ORANGE` or `YELLOW`. Confirm compile.

**Step 2: Verify**

Run:

```bash
make
make test
```

Expected: build/tests pass.

**Step 3: Commit**

```bash
git add src/render/renderer.c
git commit -m "feat: render Venus distinctly"
```

---

### Task 10: Switch default app scene to Sun + Mercury + Venus

**Objective:** Make the app launch the Venus iteration by default.

**Files:**
- Modify: `src/main.c`
- Test: build/test/smoke

**Step 1: Update initializer**

Change:

```c
SolarSystem system = solar_system_create_sun_mercury();
```

to:

```c
SolarSystem system = solar_system_create_sun_mercury_venus();
```

**Step 2: Update overlay title/text**

Change title to:

```c
DrawText("Solar System Simulator - Milestone 3: Venus", 20, 20, 20, RAYWHITE);
```

Update body description to `Sun + Mercury + Venus`.

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
git commit -m "feat: launch Venus simulation"
```

---

### Task 11: Document Venus, view modes, and camera focus

**Objective:** Update README to describe the new body and presentation controls accurately.

**Files:**
- Modify: `README.md`

**Step 1: Update milestone section**

Rename current milestone section to:

```markdown
## Milestone 3: Foundation + Sun + Mercury + Venus
```

State current behavior:

- renders exactly Sun, Mercury, and Venus;
- Mercury and Venus start at perihelion using vis-viva tangential speeds;
- Sun remains fixed;
- `V` toggles illustrative vs real-scale visualization;
- `Tab` or `C` cycles camera focus across all bodies.

**Step 2: Add Venus data**

Extend the body table and orbital values:

```markdown
| Venus | `4.8675e24 kg` | `6051800 m` | perihelion position and tangential speed |
```

Include:

- semi-major axis: `108208000000 m`
- eccentricity: `0.006772`
- perihelion distance: `107475215424.0 m`
- perihelion speed: `35259.30808092215 m/s`

**Step 3: Add controls section**

```markdown
## Controls

- `V`: toggle visualization mode.
  - Illustrative: physical orbital positions with clamped/enlarged visible body radii.
  - Real scale: physical orbital positions and physical radii under the same render scale; planets may be nearly invisible.
- `Tab` or `C`: cycle camera focus across Sun, Mercury, and Venus.
```

**Step 4: Update roadmap**

Remove Venus from next planned iteration and make Earth next:

```markdown
1. Earth
2. Moon
3. Mars
...
```

**Step 5: Verify README accuracy**

Check that README does not claim:

- accurate ephemerides;
- barycentric Sun motion;
- relativistic precession;
- any body beyond Sun, Mercury, and Venus exists;
- real-scale mode makes all bodies visually easy to see.

**Step 6: Commit**

```bash
git add README.md
git commit -m "docs: document Venus iteration"
```

---

### Task 12: Final acceptance checks and review

**Objective:** Confirm the Venus iteration meets all acceptance criteria before final commit/push/report.

**Files:**
- No intended source changes unless review finds issues.

**Step 1: Run full checks**

```bash
make clean
make
make test
set +e
timeout 3s make run > /tmp/solar-venus-run.log 2>&1
run_code=$?
set -e
if [ "$run_code" -ne 124 ] && [ "$run_code" -ne 0 ]; then
  cat /tmp/solar-venus-run.log
  exit "$run_code"
fi
grep -E 'Initializing raylib|Device initialized successfully|PLATFORM: .*Initialized successfully' /tmp/solar-venus-run.log
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
- Code quality reviewer checks simplicity, C style, test coverage, no scope creep, render/sim isolation, and camera control implementation.

**Step 3: Fix any review issues**

If review finds blocking issues, fix them and rerun the full checks.

**Step 4: Final commit/push**

If tasks were committed incrementally, push `main`:

```bash
git status --short
git push origin main
git rev-list --left-right --count HEAD...origin/main
```

If implementation was kept in one final commit instead, use:

```bash
git add README.md src/main.c src/render/renderer.c src/render/renderer.h src/sim/constants.h src/sim/solar_system.c src/sim/solar_system.h tests/test_solar_system.c
git commit -m "feat: add Venus simulation milestone"
git push origin main
git rev-list --left-right --count HEAD...origin/main
```

Expected after push: `0 0`.

---

## Acceptance Criteria for Venus + View Modes Iteration

- `make` builds `build/solar-system-simulator`.
- `make test` runs all C test binaries and passes.
- bounded `make run` initializes raylib successfully; on a desktop, `make run` opens the window.
- The default scene contains exactly three celestial bodies: Sun, Mercury, and Venus.
- Sun remains fixed at the origin.
- Mercury behavior remains intact.
- Venus uses real mass and radius in simulation data.
- Venus initial position uses perihelion distance derived from semi-major axis and eccentricity.
- Venus initial velocity uses a tangential perihelion speed derived from the vis-viva equation.
- Venus is non-fixed and moves under Newtonian gravity.
- Tests prove Venus acceleration points toward the Sun at perihelion.
- Tests prove Venus moves after stepping and remains approximately bound/periodic over one orbit.
- Rendering scale remains isolated from simulation units.
- App supports two visualization modes:
  - illustrative/default mode with visible body-radius clamping;
  - real-scale mode with physical radius scaling and no radius clamp.
- App can cycle camera focus across every simulated body, not only the Sun.
- Overlay displays current body count, view mode, camera focus target, elapsed time, and physics/render scale notes.
- README documents Venus scope, assumptions, commands, view modes, camera controls, and next planned body iteration.

---

## Risks and Mitigations

| Risk | Mitigation |
|---|---|
| Real-scale mode makes planets invisible | Document it as physically expected; keep illustrative mode as default |
| `UpdateCamera(CAMERA_ORBITAL)` overwrites dynamic focus target | Set `camera.target` after `UpdateCamera`, or replace with minimal app-local orbit handling only if needed |
| Keyboard focus cycling goes out of bounds if body count changes | Use modulo helper based on `system.body_count` |
| Renderer grows string-name color checks | Accept for this milestone; revisit with a render style table once more bodies exist |
| Venus orbit test becomes slow | Use 12-hour timestep and loose tolerance; it is a plausibility regression, not an ephemeris test |
| Capacity exact-value test breaks on next iteration | Expected; each body iteration updates the capacity deliberately |
| Real-scale radius returns sub-pixel or zero-looking spheres | Keep build/smoke as acceptance; do not corrupt simulation values to compensate |
| Scope creep into general UI/menu framework | Use only `V`, `Tab`, and `C` key handling in `src/main.c` |

---

## Open Questions for Later Iterations

These do not block Venus:

1. Should Earth start at perihelion, or should the project switch to a common epoch state vector before adding Earth/Moon?
2. Should the camera eventually support named focus selection through number keys once body count grows?
3. Should orbital traces be added as diagnostics after Earth/Moon exist?
4. Should the render layer replace string-based color checks with a body style table?
5. Should a future milestone split app input/camera state out of `src/main.c` once controls grow beyond a few keys?
6. Should a later physics-hardening milestone track energy/angular-momentum drift over many orbits?

---

## Implementation Handoff

Plan complete for the Venus + view modes + camera focus iteration. Execute using subagent-driven-development: fresh implementation subagent per task, spec review first, code-quality review second, final integration review, and full acceptance checks before committing/pushing. Stop after Venus and the requested presentation controls are implemented; do not add Earth or any other body in this iteration.
