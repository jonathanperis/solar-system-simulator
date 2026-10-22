#include "require_assert.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "app/simulation_session.h"
#include "sim/constants.h"
#include "sim/scene_epoch.h"

static void assert_same_motion(const SolarSystem *a, const SolarSystem *b)
{
    assert(a->elapsed_seconds == b->elapsed_seconds);
    for (size_t i = 0; i < a->body_count; ++i) {
        assert(vec3d_length(vec3d_sub(a->bodies[i].position_m, b->bodies[i].position_m)) == 0.0);
        assert(vec3d_length(vec3d_sub(a->bodies[i].velocity_mps, b->bodies[i].velocity_mps)) == 0.0);
    }
}

static void test_playback_pause_step_speed_and_reset(void)
{
    SimulationSession session = simulation_session_create();
    SolarSystem initial = session.system;
    simulation_session_update(&session, 20.0 / SOLAR_DAY_SECONDS);
    assert(session.clock.pending_seconds == 5.0);
    session.paused = true;
    SolarSystem paused = session.system;
    size_t points = body_trails_point_count(&session.trails, 1);
    simulation_session_update(&session, 100.0);
    assert_same_motion(&session.system, &paused);
    assert(body_trails_point_count(&session.trails, 1) == points);
    simulation_session_single_step(&session);
    solar_system_step(&paused, 15.0);
    assert_same_motion(&session.system, &paused);
    assert(session.clock.pending_seconds == 5.0);
    session.paused = false;
    simulation_session_single_step(&session);
    assert_same_motion(&session.system, &paused);

    const double rates[] = {3600.0, 86400.0, 432000.0, 864000.0, 1296000.0};
    for (int preset = 0; preset < 5; ++preset) {
        simulation_session_set_speed(&session, preset);
        simulation_session_reset(&session);
        simulation_session_update(&session, 300.0 / rates[preset]);
        SolarSystem expected = initial;
        for (int i = 0; i < 20; ++i) solar_system_step(&expected, 15.0);
        assert_same_motion(&session.system, &expected);
    }
    simulation_session_select_body(&session, 6);
    session.paused = true;
    simulation_session_update(&session, 100.0);
    simulation_session_reset(&session);
    assert_same_motion(&session.system, &initial);
    assert(session.paused && session.speed_preset == 4 && session.selected_body_index == 6);
    assert(session.clock.pending_seconds == 0.0);
    assert(body_trails_point_count(&session.trails, 6) == 1);
    assert(session.trails.sample_interval_seconds == 300.0);
    simulation_session_select_body(&session, -1);
    simulation_session_set_speed(&session, 5);
    assert(session.selected_body_index == 6 && session.speed_preset == 4);
    simulation_session_destroy(&session);
}

static void test_inspector_uses_parent_ids_and_relative_si_motion(void)
{
    SimulationSession session = simulation_session_create();
    Body swap = session.system.bodies[0];
    session.system.bodies[0] = session.system.bodies[5];
    session.system.bodies[5] = swap;
    simulation_session_select_body(&session, 6);
    BodyInspection inspection = simulation_session_inspect(&session);
    assert(inspection.has_parent && strcmp(inspection.parent_name, "Mars") == 0);
    /* Phobos starts at its Horizons 2026-06-09 state relative to Mars, which
     * must lie on its own orbit: between periareion and apoareion. */
    Vec3d phobos_r, phobos_v;
    assert(scene_epoch_state(BODY_ID_PHOBOS, &phobos_r, &phobos_v));
    assert(fabs(inspection.distance_m - vec3d_length(phobos_r)) < 0.0001);
    assert(inspection.distance_m > SOLAR_PHOBOS_PERIAREION_M * 0.99);
    assert(inspection.distance_m < SOLAR_PHOBOS_SEMI_MAJOR_AXIS_M * (1.0 + SOLAR_PHOBOS_ECCENTRICITY) * 1.01);
    assert(inspection.mass_kg == session.system.bodies[6].mass_kg);
    assert(inspection.radius_m == session.system.bodies[6].radius_m);
    double speed = inspection.speed_mps;
    Vec3d boost = {10000.0, -2000.0, 5000.0};
    for (size_t i = 0; i < session.system.body_count; ++i) {
        session.system.bodies[i].velocity_mps = vec3d_add(session.system.bodies[i].velocity_mps, boost);
    }
    assert(fabs(simulation_session_inspect(&session).speed_mps - speed) < 1e-9);
    simulation_session_select_body(&session, 5);
    inspection = simulation_session_inspect(&session);
    assert(!inspection.has_parent && strcmp(inspection.name, "Sun") == 0);
    simulation_session_destroy(&session);
}

static void test_session_exposes_jupiter_and_appended_saturn(void)
{
    SimulationSession session = simulation_session_create();

    assert(session.system.body_count == SOLAR_CORE_SCENE_BODY_COUNT);
    /* Small moons are not in the main scene; their family scene has them. */
    assert(simulation_session_find_body(&session, "s/2021 j 8", 0) == -1);
    assert(simulation_session_find_body(&session, "Galilean moons", 0) == 10);
    assert(simulation_session_find_body(&session, "Galilean moons", 11) == 11);
    assert(simulation_session_find_body(&session, "no such body", 0) == -1);
    simulation_session_select_body(&session, 9);
    BodyInspection inspection = simulation_session_inspect(&session);
    assert(strcmp(inspection.name, "Jupiter") == 0);
    assert(strcmp(inspection.parent_name, "Sun") == 0);
    /* The Jovian family barycenter takes Horizons' 2026-06-09 state (SPEC
     * A92); Jupiter is displaced opposite its major moons by under 300 km. */
    Vec3d barycenter, barycenter_v;
    scene_epoch_planet_state(4, &barycenter, &barycenter_v);
    assert(fabs(inspection.distance_m - vec3d_length(barycenter)) < 3.0e5);
    assert(inspection.mass_kg == SOLAR_JUPITER_MASS_KG);
    assert(inspection.radius_m == SOLAR_JUPITER_RADIUS_M);
    assert(simulation_session_find_body(&session, "saturn", 0) == 14);
    simulation_session_select_body(&session, 14);
    inspection = simulation_session_inspect(&session);
    assert(strcmp(inspection.name, "Saturn") == 0);
    assert(strcmp(inspection.parent_name, "Sun") == 0);
    /* Saturn's family barycenter (mostly Titan) shifts it by ~290 km. */
    scene_epoch_planet_state(5, &barycenter, &barycenter_v);
    assert(fabs(inspection.distance_m - vec3d_length(barycenter)) < 3.0e5);
    assert(inspection.mass_kg == SOLAR_SATURN_MASS_KG);
    assert(inspection.radius_m == SOLAR_SATURN_RADIUS_M);
    session.paused = true;
    simulation_session_reset(&session);
    assert(session.selected_body_index == 14);
    assert(body_trails_point_count(&session.trails, 14) == 1);

    /* A family scene is a fixed 15 s Verlet scene that opens on its planet. */
    assert(!simulation_session_start_lesson(&session, LESSON_SATURN_SYSTEM, 1, PHYSICS_EULER, 15));
    assert(!simulation_session_start_lesson(&session, LESSON_SATURN_SYSTEM, 1.5, PHYSICS_VERLET, 15));
    assert(simulation_session_start_lesson(&session, LESSON_JUPITER_SYSTEM, 1, PHYSICS_VERLET, 15));
    assert(session.system.body_count == 9 + SOLAR_JOVIAN_MOON_COUNT && session.selected_body_index == 5);
    assert(simulation_session_find_body(&session, "s/2021 j 8", 0) == 123);
    assert(simulation_session_start_lesson(&session, LESSON_SATURN_SYSTEM, 1, PHYSICS_VERLET, 15));
    assert(session.system.body_count == SOLAR_SYSTEM_BODY_CAPACITY && session.selected_body_index == 6);
    simulation_session_destroy(&session);
}

static void test_overloaded_playback_retains_time_and_freezes_while_paused(void)
{
    /* Jupiter's family scene carries the unknown-mass small moons. */
    SimulationSession session = simulation_session_create();
    assert(simulation_session_start_lesson(&session, LESSON_JUPITER_SYSTEM, 1, PHYSICS_VERLET, 15));
    simulation_session_set_speed(&session, 4);
    simulation_session_update(&session, 1.0);
    assert(session.system.elapsed_seconds == SOLAR_APP_MAX_STEPS_PER_UPDATE * 15.0);
    assert(session.clock.pending_seconds == 1296000.0 - session.system.elapsed_seconds);
    assert(session.achieved_time_scale == session.system.elapsed_seconds);
    double pending = session.clock.pending_seconds;
    session.paused = true;
    simulation_session_update(&session, 2.0);
    assert(session.clock.pending_seconds == pending);
    simulation_session_select_body(&session, 123);
    BodyInspection body = simulation_session_inspect(&session);
    assert(body.mass_quality == PHYSICAL_UNKNOWN && body.radius_quality == PHYSICAL_UNKNOWN);
    simulation_session_reset(&session);
    assert(session.selected_body_index == 123 && session.speed_preset == 4 && session.paused);
    assert(session.clock.pending_seconds == 0 && session.achieved_time_scale == 0);
    assert(body_trails_point_count(&session.trails, 123) == 1);
    simulation_session_destroy(&session);
}

static void test_stalled_frame_is_discarded_but_slow_frames_keep_pending_time(void)
{
    SimulationSession session = simulation_session_create();
    simulation_session_set_speed(&session, 4);
    /* A laptop that slept for eight hours with the window visible delivers one
     * huge frame. Integrating it would queue ~3.7e10 s of catch-up work. */
    simulation_session_update(&session, 8.0 * 3600.0);
    assert(session.clock.ticks == 0 && session.clock.pending_seconds == 0);
    assert(session.rate_real_seconds == 0 && session.rate_sim_seconds == 0);
    simulation_session_update(&session, NAN);
    assert(session.clock.ticks == 0 && session.clock.pending_seconds == 0);
    /* Playback resumes normally on the next frame. */
    simulation_session_update(&session, 1.0 / 60.0);
    assert(session.clock.ticks == 1440);
    /* A slow frame at the threshold is still honoured: work is capped and the
     * remainder stays pending rather than being hidden. */
    simulation_session_update(&session, SOLAR_APP_STALL_FRAME_SECONDS);
    assert(session.clock.ticks == 1440 + SOLAR_APP_MAX_STEPS_PER_UPDATE);
    assert(session.clock.pending_seconds > 0);
    simulation_session_destroy(&session);
}

static void test_lessons_reset_configuration_and_exclude_background_time(void)
{
    SimulationSession session = simulation_session_create();
    assert(simulation_session_start_lesson(&session, LESSON_EARTH_MOON, 1.1, PHYSICS_EULER, 30));
    SolarSystem initial = session.system;
    assert(session.clock.step_seconds == 30 && session.clock.integrator == PHYSICS_EULER);
    assert(session.system.body_count == 2 && session.lesson == LESSON_EARTH_MOON);
    session.paused = true;
    simulation_session_single_step(&session);
    assert(session.clock.ticks == 1 && session.system.elapsed_seconds == 30);
    session.paused = false;
    simulation_session_set_background(&session, true);
    simulation_session_update(&session, 60);
    simulation_session_set_background(&session, false);
    simulation_session_update(&session, 60); /* First resumed frame contains hidden wall time. */
    assert(session.clock.ticks == 1);
    simulation_session_update(&session, 30.0 / SOLAR_DAY_SECONDS);
    assert(session.clock.ticks == 2);
    assert(!simulation_session_start_lesson(&session, LESSON_CORE, 1, PHYSICS_EULER, 15));
    assert(!simulation_session_start_lesson(&session, LESSON_CORE, 1, PHYSICS_VERLET, 300));
    assert(!simulation_session_start_lesson(&session, LESSON_CIRCULAR, NAN, PHYSICS_VERLET, 15));
    assert(session.lesson == LESSON_EARTH_MOON && session.clock.ticks == 2);
    simulation_session_reset(&session);
    assert_same_motion(&session.system, &initial);
    assert(session.clock.ticks == 0 && session.clock.step_seconds == 30);
    BodyInspection body = simulation_session_inspect(&session);
    assert(body.acceleration_mps2 > 0 && body.specific_energy_jpkg < 0);
    assert(simulation_session_start_lesson(&session, LESSON_CIRCULAR, 1, PHYSICS_VERLET, 200));
    assert(session.trails.sample_interval_seconds == 400);
    session.paused = true;
    simulation_session_single_step(&session);
    simulation_session_single_step(&session);
    assert(session.trails.last_sample_seconds == 400);
    assert(simulation_session_start_lesson(&session, LESSON_CIRCULAR, 1, PHYSICS_VERLET, 1.1));
    for (int i = 0; i < 20000; ++i) simulation_session_single_step(&session);
    double spacing = session.trails.sample_interval_seconds;
    assert(fabs(session.trails.last_sample_seconds - floor(session.system.elapsed_seconds / spacing) * spacing) < 1e-8);
    simulation_session_demo(&session);
    assert(session.clock.step_seconds == 15 && session.clock.integrator == PHYSICS_VERLET);
    assert(session.lesson == LESSON_CORE && session.system.body_count == SOLAR_CORE_SCENE_BODY_COUNT);
    simulation_session_destroy(&session);
}

static void test_non_finite_state_is_detected(void)
{
    SimulationSession session = simulation_session_create();
    assert(simulation_session_state_is_finite(&session));
    session.system.bodies[3].velocity_mps.y = NAN;
    assert(!simulation_session_state_is_finite(&session));
    simulation_session_reset(&session);
    session.system.bodies[5].position_m.x = INFINITY;
    assert(!simulation_session_state_is_finite(&session));
    simulation_session_destroy(&session);
}

/* Presets built from the dated main scene run on the 2026-06-09 calendar;
 * the analytic lessons count seconds from zero. */
static void test_dated_sessions_are_the_epoch_presets(void)
{
    SimulationSession session = simulation_session_create();
    assert(simulation_session_is_dated(&session));
    assert(simulation_session_start_lesson(&session, LESSON_BARYCENTRIC_CORE, 1, PHYSICS_VERLET, 15));
    assert(simulation_session_is_dated(&session));
    assert(simulation_session_start_lesson(&session, LESSON_CIRCULAR, 1, PHYSICS_VERLET, 15));
    assert(!simulation_session_is_dated(&session));
    assert(simulation_session_start_lesson(&session, LESSON_PLUTO_SYSTEM, 1, PHYSICS_VERLET, 15));
    assert(simulation_session_is_dated(&session));
    simulation_session_destroy(&session);
}

/* Run until (SPEC A133): the session plays at its fastest preset to a target
 * time, never past it, and pauses within one tick of it; it never skips
 * time or enlarges a step, so the physics is exactly what playing would do. */
static void test_run_until_stops_on_the_target_within_one_tick(void)
{
    SimulationSession session = simulation_session_create();
    double step = simulation_clock_step_seconds(&session.clock);
    double target = 3.25 * SOLAR_DAY_SECONDS + 7.0;
    simulation_session_run_until(&session, target);
    assert(!session.paused && session.speed_preset == SOLAR_SPEED_PRESET_COUNT - 1);
    for (int frame = 0; frame < 2000 && !session.paused; ++frame) {
        simulation_session_update(&session, 1.0 / 60.0);
        assert(session.system.elapsed_seconds <= target + 1e-6);
    }
    /* Queued time left by a slow frame (the per-update step cap) must not
     * carry a run past its target either. */
    {
        SimulationSession queued = simulation_session_create();
        double goal = 40.0 * step + 3.0;
        queued.clock.pending_seconds = 100.0 * step;
        simulation_session_run_until(&queued, goal);
        for (int frame = 0; frame < 100 && !queued.paused; ++frame) {
            simulation_session_update(&queued, 1.0 / 60.0);
            assert(queued.system.elapsed_seconds <= goal + 1e-6);
        }
        assert(queued.paused && queued.system.elapsed_seconds > goal - step - 1e-6);
        simulation_session_destroy(&queued);
    }
    assert(session.paused);
    assert(session.system.elapsed_seconds > target - step - 1e-6);
    assert(session.run_until_seconds < 0);
    /* A target already behind the clock pauses at once and moves nothing. */
    double now = session.system.elapsed_seconds;
    simulation_session_run_until(&session, now - 100);
    assert(session.paused && session.system.elapsed_seconds == now);
    /* Pausing or resetting cancels a run. */
    simulation_session_run_until(&session, now + SOLAR_DAY_SECONDS);
    session.paused = true;
    simulation_session_update(&session, 1.0 / 60.0);
    assert(session.run_until_seconds < 0 && session.system.elapsed_seconds == now);
    simulation_session_destroy(&session);
}

/* From Earth (A136): the selected body's distance from Earth's centre in SI
 * state, for the inspector's distance and light-time line. */
static void test_inspection_measures_the_distance_from_earth(void)
{
    SimulationSession session = simulation_session_create();
    int earth = -1, mars = -1, moon = -1;
    for (size_t i = 0; i < session.system.body_count; ++i) {
        if (session.system.bodies[i].id == BODY_ID_EARTH) earth = (int)i;
        if (session.system.bodies[i].id == BODY_ID_MARS) mars = (int)i;
        if (session.system.bodies[i].id == BODY_ID_MOON) moon = (int)i;
    }
    assert(earth >= 0 && mars >= 0 && moon >= 0);
    BodyInspection at_mars = simulation_session_inspect_body(&session, (size_t)mars);
    double expected = vec3d_length(vec3d_sub(session.system.bodies[mars].position_m, session.system.bodies[earth].position_m));
    assert(fabs(at_mars.earth_distance_m - expected) <= 1e-9 * expected);
    BodyInspection at_moon = simulation_session_inspect_body(&session, (size_t)moon);
    assert(at_moon.earth_distance_m > 356e6 && at_moon.earth_distance_m < 407e6);
    assert(isnan(simulation_session_inspect_body(&session, (size_t)earth).earth_distance_m));
    simulation_session_destroy(&session);
    /* A lesson without Earth has no such line. */
    SimulationSession lesson = simulation_session_create();
    assert(simulation_session_start_lesson(&lesson, LESSON_CIRCULAR, 1.0, PHYSICS_VERLET, 15.0));
    bool has_earth = false;
    for (size_t i = 0; i < lesson.system.body_count; ++i) has_earth = has_earth || lesson.system.bodies[i].id == BODY_ID_EARTH;
    if (!has_earth) assert(isnan(simulation_session_inspect_body(&lesson, 0).earth_distance_m));
    simulation_session_destroy(&lesson);
}

/* Seen from Earth (A145): a body's apparent disc and its elongation, the
 * Sun-Earth-body angle that decides whether it can be seen at night. */
static void test_inspection_measures_disc_and_elongation_from_earth(void)
{
    SimulationSession session = simulation_session_create();
    int sun = -1, earth = -1, jupiter = -1, moon = -1;
    for (size_t i = 0; i < session.system.body_count; ++i) {
        BodyId id = session.system.bodies[i].id;
        if (session.system.bodies[i].kind == BODY_KIND_STAR) sun = (int)i;
        if (id == BODY_ID_EARTH) earth = (int)i;
        if (id == BODY_ID_JUPITER) jupiter = (int)i;
        if (id == BODY_ID_MOON) moon = (int)i;
    }
    assert(sun >= 0 && earth >= 0 && jupiter >= 0 && moon >= 0);
    const Body *e = &session.system.bodies[earth], *j = &session.system.bodies[jupiter], *s = &session.system.bodies[sun];
    BodyInspection at_jupiter = simulation_session_inspect_body(&session, (size_t)jupiter);
    Vec3d to_j = vec3d_sub(j->position_m, e->position_m), to_s = vec3d_sub(s->position_m, e->position_m);
    double expected = acos(vec3d_dot(to_j, to_s) / (vec3d_length(to_j) * vec3d_length(to_s))) * 180.0 / acos(-1.0);
    assert(fabs(at_jupiter.elongation_deg - expected) < 1e-9);
    /* Jupiter's disc is 30-50 arcseconds; the Moon's about half a degree. */
    assert(at_jupiter.angular_diameter_arcsec > 30 && at_jupiter.angular_diameter_arcsec < 50);
    BodyInspection at_moon = simulation_session_inspect_body(&session, (size_t)moon);
    assert(at_moon.angular_diameter_arcsec > 29 * 60 && at_moon.angular_diameter_arcsec < 34 * 60);
    /* The lit fraction (A146), (1 + cos i) / 2 with i the Sun-body-Earth
     * phase angle. For the Moon i is close to 180 degrees minus its
     * elongation, so a Moon 90 degrees from the Sun is half lit. */
    const Body *m = &session.system.bodies[moon];
    Vec3d moon_to_sun = vec3d_sub(s->position_m, m->position_m), moon_to_earth = vec3d_sub(e->position_m, m->position_m);
    double phase = acos(vec3d_dot(moon_to_sun, moon_to_earth) / (vec3d_length(moon_to_sun) * vec3d_length(moon_to_earth)));
    assert(fabs(at_moon.illuminated_fraction - (1 + cos(phase)) / 2) < 1e-12);
    double approx = (1 - cos(at_moon.elongation_deg * acos(-1.0) / 180.0)) / 2;
    assert(fabs(at_moon.illuminated_fraction - approx) < 0.01);
    /* Jupiter, outside Earth's orbit, always shows nearly all its disc. */
    assert(at_jupiter.illuminated_fraction > 0.98 && at_jupiter.illuminated_fraction <= 1.0);
    assert(isnan(simulation_session_inspect_body(&session, (size_t)sun).illuminated_fraction));
    /* No elongation for the Sun itself or for Earth; no disc for Earth. */
    assert(isnan(simulation_session_inspect_body(&session, (size_t)sun).elongation_deg));
    assert(simulation_session_inspect_body(&session, (size_t)sun).angular_diameter_arcsec > 31 * 60);
    assert(isnan(simulation_session_inspect_body(&session, (size_t)earth).angular_diameter_arcsec));
    assert(isnan(simulation_session_inspect_body(&session, (size_t)earth).elongation_deg));
    simulation_session_destroy(&session);
}

int main(void)
{
    test_inspection_measures_disc_and_elongation_from_earth();
    test_inspection_measures_the_distance_from_earth();
    test_run_until_stops_on_the_target_within_one_tick();
    test_dated_sessions_are_the_epoch_presets();
    test_non_finite_state_is_detected();
    test_stalled_frame_is_discarded_but_slow_frames_keep_pending_time();
    test_lessons_reset_configuration_and_exclude_background_time();
    test_overloaded_playback_retains_time_and_freezes_while_paused();
    test_playback_pause_step_speed_and_reset();
    test_inspector_uses_parent_ids_and_relative_si_motion();
    test_session_exposes_jupiter_and_appended_saturn();
    puts("test_simulation_session passed");
    return 0;
}
