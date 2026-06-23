# Martian Moon Orbit Plane Debug Plan

> **For Hermes:** Use systematic-debugging before changing physics; reproduce the reported long-run visual symptom, add a regression test, then make the smallest physics-initialization change.

**Goal:** Fix the long-run Phobos/Deimos orbit/trail presentation so fast-followed orbits remain planar and visually expected instead of drawing vertical/box-like paths after hundreds of simulated days.

**Architecture:** Keep `src/sim/` as SI-unit physics and keep rendering as a boundary. The likely fix is in initial conditions for Mars-relative satellites, not in trail storage or renderer scaling: satellites should orbit in the same X/Z ecliptic plane used by the planets unless we intentionally model inclinations.

**Tech Stack:** C11, raylib, deterministic simulation tests, existing `make test` harness.

---

## Evidence snapshot

- Repo/path: `/opt/data/github/jonathanperis/solar-system-simulator`
- Branch/ref at start: `main` at `ba19fc3`, then work branch `fix/martian-moon-orbit-plane`
- Working tree before changes: clean
- User screenshot: elapsed `550.62` days, time scale `86400 simulation seconds / real second`, illustrative view, Sun focus. The visible Phobos/Deimos trails form tall rectangular/vertical-looking bands around Mars instead of readable in-plane satellite loops.
- Numeric fast-run reproduction: a 550.62-day headless probe with app-equivalent 300-second substeps shows distances remain stable, so this is not moon escape or integrator blow-up:
  - Phobos parent distance: about `9235–9541 km`
  - Deimos parent distance: about `23452–23471 km`
  - Planet heliocentric distances remain bounded
- Code evidence: `src/sim/solar_system.c` initializes Phobos/Deimos relative velocities on the `Y` axis while planet orbits live on the `X/Z` plane. That makes the Martian moon orbital planes vertical to the grid/ecliptic, which matches the screenshot's unexpected vertical trails.

## Overall progress

| Lane | Evidence checked | Implementation status | Validation status |
| --- | --- | --- | --- |
| Reproduce | Screenshot + 550-day numeric fast probe | Done | Distances stable; issue isolated to orientation, not escape |
| Regression | Add test for Martian satellite orbital plane | Pending | Should fail before fix |
| Fix | Change Phobos/Deimos relative velocity from Y-axis to Z-axis with prograde signs | Pending | Existing short-step tests updated |
| Verify | Long-run probe, `make test`, build, headless raylib smoke, diff check | Pending | Run after fix |

---

## Task 1: Add an orbit-plane regression for Mars satellites

**Objective:** Prove the reported visual problem in a simulation-only test before fixing it.

**Files:**
- Modify: `tests/test_solar_system.c`

**Steps:**
1. Add a test near the existing Phobos/Deimos initial-condition tests that creates the full scene and asserts both Martian moons start with zero parent-relative `Y` velocity and nonzero parent-relative `Z` velocity.
2. Run `make build/tests/test_solar_system && build/tests/test_solar_system`.
3. Expected before fix: failure because current Phobos/Deimos relative velocity is on `Y`.

## Task 2: Move Phobos/Deimos into the X/Z orbital plane

**Objective:** Make the Martian moons follow the same ecliptic plane convention as the rest of the simulator.

**Files:**
- Modify: `src/sim/solar_system.c`
- Modify: `tests/test_solar_system.c`

**Steps:**
1. In `solar_system_create_phobos_at_periareion_near_mars`, change relative velocity from `{0.0, speed, 0.0}` to `{0.0, 0.0, speed}`.
2. In `solar_system_create_deimos_at_periareion_near_mars`, change relative velocity from `{0.0, -speed, 0.0}` to `{0.0, 0.0, -speed}`.
3. Update the existing short-step tests to expect `z` prograde/retrograde movement instead of `y`.
4. Keep all physics SI state unchanged otherwise.

## Task 3: Fast-follow the orbits headlessly after the fix

**Objective:** Re-run the user's scenario without a GUI and verify the fix after hundreds of simulated days.

**Files:**
- Temporary: `/tmp/orbit_probe.c` only; do not commit it.

**Steps:**
1. Compile a temporary C probe against `src/sim/*.c`.
2. Simulate `550.62` days with `300` second steps, matching the app's max physics substep.
3. Verify Phobos/Deimos parent distances stay within expected peri/apo ranges and their `Y` coordinates stay at `0`, confirming they no longer draw vertical trails in the default no-inclination model.

## Task 4: Full verification and delivery

**Objective:** Ensure the targeted fix does not regress the app.

**Commands:**
```bash
make clean && make && make test
timeout 3s make run > /tmp/solar-run.log 2>&1 || test $? -eq 124
grep -E 'Initializing raylib|Device initialized successfully|PLATFORM: .*Initialized successfully' /tmp/solar-run.log
git diff --check
```

**Expected:** Build succeeds, all tests pass, headless raylib initializes, and whitespace check is clean.

## Acceptance criteria

- Phobos and Deimos initialize in the X/Z orbital plane, matching the grid/ecliptic convention used by Mercury, Venus, Earth, Moon, and Mars.
- Fast-run simulation through the screenshot's elapsed time remains numerically stable.
- Tests cover the orbit-plane convention so this specific visual regression cannot return silently.
- No renderer hacks or simulation/render boundary violations are introduced.
