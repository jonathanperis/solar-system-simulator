#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "app/body_trails.h"
#include "app/simulation_step.h"
#include "sim/constants.h"
#include "sim/solar_system.h"

static void assert_vec3d_equal(Vec3d actual, Vec3d expected)
{
    assert(actual.x == expected.x);
    assert(actual.y == expected.y);
    assert(actual.z == expected.z);
}

static void test_trail_sampling_is_independent_of_physics_step_size(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos_vesta();
    BodyTrails trails = body_trails_create();
    SimulationClock clock = {0};

    body_trails_record_system(&trails, &system);
    solar_app_step_system_with_trails(&system, &trails, &clock, SOLAR_DAY_SECONDS);

    assert(body_trails_point_count(&trails, 0) == 0);
    assert(body_trails_point_count(&trails, 1) == 1 + 288);
    assert(body_trails_point_count(&trails, 3) == 1 + 288);
    assert(body_trails_point_count(&trails, 5) == 1 + 288);
    assert(body_trails_point_count(&trails, 6) == 1 + 288);
    assert(body_trails_point_count(&trails, 7) == 1 + 288);
    assert(body_trails_point_count(&trails, 8) == 1 + 288);
    assert_vec3d_equal(body_trails_point_at(&trails, 6, body_trails_point_count(&trails, 6) - 1), system.bodies[6].position_m);
    assert_vec3d_equal(body_trails_point_at(&trails, 7, body_trails_point_count(&trails, 7) - 1), system.bodies[7].position_m);
    assert_vec3d_equal(body_trails_point_at(&trails, 8, body_trails_point_count(&trails, 8) - 1), system.bodies[8].position_m);

    body_trails_destroy(&trails);
}

static void test_frame_partitioning_preserves_state_and_pending_time(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos();
    BodyTrails trails = body_trails_create();
    SolarSystem partitioned = system;
    BodyTrails partitioned_trails = body_trails_create();
    SimulationClock clock = {0};
    SimulationClock partitioned_clock = {0};

    body_trails_record_system(&trails, &system);
    body_trails_record_system(&partitioned_trails, &partitioned);
    solar_app_step_system_with_trails(&system, &trails, &clock, 650.0);
    for (int i = 0; i < 65; ++i) {
        solar_app_step_system_with_trails(&partitioned, &partitioned_trails, &partitioned_clock, 10.0);
    }

    assert(system.elapsed_seconds == 645.0);
    assert(clock.pending_seconds == 5.0);
    assert(partitioned_clock.pending_seconds == clock.pending_seconds);
    for (size_t i = 0; i < system.body_count; ++i) {
        assert_vec3d_equal(system.bodies[i].position_m, partitioned.bodies[i].position_m);
        assert_vec3d_equal(system.bodies[i].velocity_mps, partitioned.bodies[i].velocity_mps);
    }
    assert(body_trails_point_count(&trails, 1) == 4);
    assert_vec3d_equal(body_trails_point_at(&trails, 1, 3), system.bodies[1].position_m);

    body_trails_destroy(&trails);
    body_trails_destroy(&partitioned_trails);
}

static void test_martian_moons_keep_phase_over_100_days(void)
{
    const double duration = 100.0 * SOLAR_DAY_SECONDS;
    for (int moon = 0; moon < 2; ++moon) {
        Body mars = solar_system_create_mars_at_perihelion();
        mars.position_m = vec3d_zero();
        mars.velocity_mps = vec3d_zero();
        Body satellite = moon == 0 ? solar_system_create_phobos_at_periareion_near_mars(&mars)
                                  : solar_system_create_deimos_at_periareion_near_mars(&mars);
        SolarSystem system = {.bodies = {mars, satellite}, .body_count = 2};
        double a = moon == 0 ? SOLAR_PHOBOS_SEMI_MAJOR_AXIS_M : SOLAR_DEIMOS_SEMI_MAJOR_AXIS_M;
        double e = moon == 0 ? SOLAR_PHOBOS_ECCENTRICITY : SOLAR_DEIMOS_ECCENTRICITY;
        double mu = SOLAR_G * (mars.mass_kg + satellite.mass_kg);
        double mean_anomaly = fmod(sqrt(mu / (a * a * a)) * duration, 2.0 * acos(-1.0));
        double eccentric_anomaly = mean_anomaly;
        for (int i = 0; i < 12; ++i) {
            eccentric_anomaly -= (eccentric_anomaly - e * sin(eccentric_anomaly) - mean_anomaly)
                / (1.0 - e * cos(eccentric_anomaly));
        }
        double direction = moon == 0 ? 1.0 : -1.0;
        Vec3d expected = {direction * a * (cos(eccentric_anomaly) - e), 0.0,
            direction * a * sqrt(1.0 - e * e) * sin(eccentric_anomaly)};

        for (double t = 0.0; t < duration; t += SOLAR_APP_MAX_PHYSICS_STEP_SECONDS) {
            solar_system_step(&system, fmin(SOLAR_APP_MAX_PHYSICS_STEP_SECONDS, duration - t));
        }
        Vec3d relative = vec3d_sub(system.bodies[1].position_m, system.bodies[0].position_m);
        Vec3d velocity = vec3d_sub(system.bodies[1].velocity_mps, system.bodies[0].velocity_mps);
        double angle_error = fabs(atan2(relative.x * expected.z - relative.z * expected.x,
            relative.x * expected.x + relative.z * expected.z)) * 180.0 / acos(-1.0);
        double energy = 0.5 * vec3d_length_squared(velocity) - mu / vec3d_length(relative);
        double expected_energy = -mu / (2.0 * a);
        printf("%s 100-day phase error: %.6f degrees\n", satellite.name, angle_error);
        fflush(stdout);
        assert(angle_error < 1.0);
        assert(fabs((energy - expected_energy) / expected_energy) < 0.000001);
    }
}

static void test_full_scene_converges_over_100_days(void)
{
    SolarSystem actual = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos_vesta();
    SolarSystem reference = actual;
    const double dt = SOLAR_APP_MAX_PHYSICS_STEP_SECONDS;
    const int parent_indices[] = {0, 0, 0, 0, 3, 0, 5, 5, 0};
    for (double t = 0.0; t < 100.0 * SOLAR_DAY_SECONDS; t += dt) {
        solar_system_step(&actual, dt);
        solar_system_step(&reference, dt * 0.5);
        solar_system_step(&reference, dt * 0.5);
    }
    for (size_t i = 1; i < actual.body_count; ++i) {
        Vec3d position = vec3d_sub(actual.bodies[i].position_m, actual.bodies[parent_indices[i]].position_m);
        Vec3d expected = vec3d_sub(reference.bodies[i].position_m, reference.bodies[parent_indices[i]].position_m);
        double relative_error = vec3d_length(vec3d_sub(position, expected)) / vec3d_length(expected);
        printf("%s 100-day half-step discrepancy: %.6f%%\n", actual.bodies[i].name, 100.0 * relative_error);
        assert(relative_error < 0.01);
    }
}

int main(void)
{
    test_martian_moons_keep_phase_over_100_days();
    test_full_scene_converges_over_100_days();
    test_trail_sampling_is_independent_of_physics_step_size();
    test_frame_partitioning_preserves_state_and_pending_time();
    puts("test_simulation_step passed");
    return 0;
}
