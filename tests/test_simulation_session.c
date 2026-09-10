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

    const double rates[] = {3600.0, 86400.0, 432000.0};
    for (int preset = 0; preset < 3; ++preset) {
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
    assert(session.paused && session.speed_preset == 2 && session.selected_body_index == 6);
    assert(session.clock.pending_seconds == 0.0);
    assert(body_trails_point_count(&session.trails, 6) == 1);
    assert(session.trails.sample_interval_seconds == 300.0);
    simulation_session_select_body(&session, -1);
    simulation_session_set_speed(&session, 3);
    assert(session.selected_body_index == 6 && session.speed_preset == 2);
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

int main(void)
{
    test_playback_pause_step_speed_and_reset();
    test_inspector_uses_parent_ids_and_relative_si_motion();
    puts("test_simulation_session passed");
    return 0;
}
