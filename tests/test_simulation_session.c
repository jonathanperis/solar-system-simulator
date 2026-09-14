#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "app/simulation_session.h"
#include "sim/constants.h"

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
    assert(fabs(inspection.distance_m - SOLAR_PHOBOS_PERIAREION_M) < 0.0001);
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

    assert(session.system.body_count == 128);
    assert(simulation_session_find_body(&session, "s/2021 j 8", 0) == 124);
    assert(simulation_session_find_body(&session, "Galilean moons", 0) == 10);
    assert(simulation_session_find_body(&session, "Galilean moons", 11) == 11);
    assert(simulation_session_find_body(&session, "no such body", 0) == -1);
    simulation_session_select_body(&session, 9);
    BodyInspection inspection = simulation_session_inspect(&session);
    assert(strcmp(inspection.name, "Jupiter") == 0);
    assert(strcmp(inspection.parent_name, "Sun") == 0);
    assert(fabs(inspection.distance_m - SOLAR_JUPITER_PERIHELION_M) < 0.001);
    assert(inspection.mass_kg == SOLAR_JUPITER_MASS_KG);
    assert(inspection.radius_m == SOLAR_JUPITER_RADIUS_M);
    assert(simulation_session_find_body(&session, "saturn", 0) == 125);
    simulation_session_select_body(&session, 125);
    inspection = simulation_session_inspect(&session);
    assert(strcmp(inspection.name, "Saturn") == 0);
    assert(strcmp(inspection.parent_name, "Sun") == 0);
    assert(fabs(inspection.distance_m - SOLAR_SATURN_PERIHELION_M) < 0.001);
    assert(inspection.mass_kg == SOLAR_SATURN_MASS_KG);
    assert(inspection.radius_m == SOLAR_SATURN_RADIUS_M);
    session.paused = true;
    simulation_session_reset(&session);
    assert(session.selected_body_index == 125);
    assert(body_trails_point_count(&session.trails, 125) == 1);
    simulation_session_destroy(&session);
}

static void test_overloaded_playback_retains_time_and_freezes_while_paused(void)
{
    SimulationSession session = simulation_session_create();
    simulation_session_set_speed(&session, 4);
    simulation_session_update(&session, 1.0);
    assert(session.system.elapsed_seconds == SOLAR_APP_MAX_STEPS_PER_UPDATE * 15.0);
    assert(session.clock.pending_seconds == 1296000.0 - session.system.elapsed_seconds);
    assert(session.achieved_time_scale == session.system.elapsed_seconds);
    double pending = session.clock.pending_seconds;
    session.paused = true;
    simulation_session_update(&session, 2.0);
    assert(session.clock.pending_seconds == pending);
    simulation_session_select_body(&session, 124);
    BodyInspection body = simulation_session_inspect(&session);
    assert(body.mass_quality == PHYSICAL_UNKNOWN && body.radius_quality == PHYSICAL_UNKNOWN);
    simulation_session_reset(&session);
    assert(session.selected_body_index == 124 && session.speed_preset == 4 && session.paused);
    assert(session.clock.pending_seconds == 0 && session.achieved_time_scale == 0);
    assert(body_trails_point_count(&session.trails, 124) == 1);
    simulation_session_destroy(&session);
}

int main(void)
{
    test_overloaded_playback_retains_time_and_freezes_while_paused();
    test_playback_pause_step_speed_and_reset();
    test_inspector_uses_parent_ids_and_relative_si_motion();
    test_session_exposes_jupiter_and_appended_saturn();
    puts("test_simulation_session passed");
    return 0;
}
