#include "require_assert.h"

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
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos_vesta_jupiter();
    BodyTrails trails = body_trails_create();
    SimulationClock clock = {0};

    body_trails_record_system(&trails, &system);
    solar_app_step_system_with_trails(&system, &trails, &clock, SOLAR_DAY_SECONDS, 5760);

    assert(body_trails_point_count(&trails, 0) == 0);
    assert(body_trails_point_count(&trails, 1) == 1 + 288);
    assert(body_trails_point_count(&trails, 3) == 1 + 288);
    assert(body_trails_point_count(&trails, 5) == 1 + 288);
    assert(body_trails_point_count(&trails, 6) == 1 + 288);
    assert(body_trails_point_count(&trails, 7) == 1 + 288);
    assert(body_trails_point_count(&trails, 8) == 1 + 288);
    assert(body_trails_point_count(&trails, 9) == 1 + 288);
    assert_vec3d_equal(body_trails_point_at(&trails, 6, body_trails_point_count(&trails, 6) - 1), system.bodies[6].position_m);
    assert_vec3d_equal(body_trails_point_at(&trails, 7, body_trails_point_count(&trails, 7) - 1), system.bodies[7].position_m);
    assert_vec3d_equal(body_trails_point_at(&trails, 8, body_trails_point_count(&trails, 8) - 1), system.bodies[8].position_m);
    assert_vec3d_equal(body_trails_point_at(&trails, 9, body_trails_point_count(&trails, 9) - 1), system.bodies[9].position_m);

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
    solar_app_step_system_with_trails(&system, &trails, &clock, 650.0, 10);
    assert(system.elapsed_seconds == 150 && clock.pending_seconds == 500);
    while (clock.pending_seconds >= 15) solar_app_step_system_with_trails(&system, &trails, &clock, 0, 10);
    for (int i = 0; i < 65; ++i) {
        solar_app_step_system_with_trails(&partitioned, &partitioned_trails, &partitioned_clock, 10.0, 10);
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

static void test_stalled_frames_are_distinguished_from_slow_frames(void)
{
    /* Ordinary and merely slow frames are real elapsed playback time. */
    assert(!simulation_frame_is_stall(0.0));
    assert(!simulation_frame_is_stall(1.0 / 60.0));
    assert(!simulation_frame_is_stall(0.5));
    assert(!simulation_frame_is_stall(SOLAR_APP_STALL_FRAME_SECONDS));
    /* Sleep, debugger pauses and corrupt clocks are not playback time. */
    assert(simulation_frame_is_stall(nextafter(SOLAR_APP_STALL_FRAME_SECONDS, INFINITY)));
    assert(simulation_frame_is_stall(8.0 * 3600.0));
    assert(simulation_frame_is_stall(-0.01));
    assert(simulation_frame_is_stall(NAN));
    assert(simulation_frame_is_stall(INFINITY));
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
        /* Prograde motion runs from periapsis toward ecliptic +Y, which is
         * simulation -Z; Deimos starts on the opposite side (-X). */
        double direction = moon == 0 ? 1.0 : -1.0;
        Vec3d expected = {direction * a * (cos(eccentric_anomaly) - e), 0.0,
            -direction * a * sqrt(1.0 - e * e) * sin(eccentric_anomaly)};

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

/* Steps a scene with the app step and with half steps, then checks every
 * body's parent-relative position agrees within 1% (V22). */
static void assert_scene_converges(SolarSystem actual, double days, bool print)
{
    SolarSystem reference = actual;
    const double dt = SOLAR_APP_MAX_PHYSICS_STEP_SECONDS;
    for (double t = 0.0; t < days * SOLAR_DAY_SECONDS; t += dt) {
        solar_system_step(&actual, dt);
        solar_system_step(&reference, dt * 0.5);
        solar_system_step(&reference, dt * 0.5);
    }
    double worst = 0;
    for (size_t i = 1; i < actual.body_count; ++i) {
        int parent_index = solar_system_parent_index(&actual, i);
        assert(parent_index >= 0);
        Vec3d position = vec3d_sub(actual.bodies[i].position_m, actual.bodies[parent_index].position_m);
        Vec3d expected = vec3d_sub(reference.bodies[i].position_m, reference.bodies[parent_index].position_m);
        double relative_error = vec3d_length(vec3d_sub(position, expected)) / vec3d_length(expected);
        if (print) printf("%s %.0f-day half-step discrepancy: %.6f%%\n", actual.bodies[i].name, days, 100.0 * relative_error);
        worst = fmax(worst, relative_error);
        assert(relative_error < 0.01);
    }
    if (!print) printf("worst %.0f-day half-step discrepancy over %zu bodies: %.6f%%\n", days, actual.body_count, 100.0 * worst);
}

static void test_full_scene_converges_over_100_days(void)
{
    assert_scene_converges(solar_system_create_current(), 100, true);
}

/* Family scenes hold up to 300 bodies; 20 days still spans dozens of orbits
 * of the fast inner moons (Pan, Cordelia, Naiad: 7-8 hours; Dimorphos: 11.8
 * hours around Didymos) and keeps the
 * sanitizer run affordable. Saturn's Janus/Epimetheus co-orbitals and the
 * Tethys/Dione trojans are included. */
static void test_family_scenes_converge_over_20_days(void)
{
    const BodyId planets[] = {BODY_ID_JUPITER, BODY_ID_SATURN, BODY_ID_URANUS, BODY_ID_NEPTUNE, BODY_ID_PLUTO, BODY_ID_DIDYMOS,
        BODY_ID_PATROCLUS};
    for (size_t k = 0; k < sizeof(planets) / sizeof(planets[0]); ++k) {
        SolarSystem family;
        assert(solar_system_create_family(planets[k], &family));
        assert_scene_converges(family, 20, false);
    }
}

int main(void)
{
    test_stalled_frames_are_distinguished_from_slow_frames();
    test_martian_moons_keep_phase_over_100_days();
    test_full_scene_converges_over_100_days();
    test_family_scenes_converge_over_20_days();
    test_trail_sampling_is_independent_of_physics_step_size();
    test_frame_partitioning_preserves_state_and_pending_time();
    puts("test_simulation_step passed");
    return 0;
}
