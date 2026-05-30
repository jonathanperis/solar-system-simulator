#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "sim/constants.h"
#include "sim/solar_system.h"

static void assert_close(double actual, double expected, double epsilon)
{
    assert(fabs(actual - expected) <= epsilon);
}

static void test_sun_body_creation_preserves_fields(void)
{
    Body sun = body_create("Sun", BODY_KIND_STAR, SOLAR_SUN_MASS_KG, SOLAR_SUN_RADIUS_M, vec3d_zero(), vec3d_zero(), true);

    assert(strcmp(sun.name, "Sun") == 0);
    assert(sun.kind == BODY_KIND_STAR);
    assert_close(sun.mass_kg, SOLAR_SUN_MASS_KG, SOLAR_SUN_MASS_KG * 1e-15);
    assert_close(sun.radius_m, SOLAR_SUN_RADIUS_M, 1e-6);
    assert(sun.fixed);
    assert_close(sun.position_m.x, 0.0, 1e-12);
    assert_close(sun.velocity_mps.x, 0.0, 1e-12);
    assert_close(sun.acceleration_mps2.x, 0.0, 1e-18);
}

static void test_sun_only_system_has_one_real_sun(void)
{
    SolarSystem system = solar_system_create_sun_only();

    assert(system.body_count == 1);
    assert_close(system.elapsed_seconds, 0.0, 1e-12);
    assert(strcmp(system.bodies[0].name, "Sun") == 0);
    assert(system.bodies[0].kind == BODY_KIND_STAR);
    assert_close(system.bodies[0].mass_kg, SOLAR_SUN_MASS_KG, SOLAR_SUN_MASS_KG * 1e-15);
    assert_close(system.bodies[0].radius_m, SOLAR_SUN_RADIUS_M, 1e-6);
    assert(system.bodies[0].fixed);
}

static void test_sun_only_step_advances_time_and_keeps_sun_fixed(void)
{
    SolarSystem system = solar_system_create_sun_only();

    solar_system_step(&system, SOLAR_DAY_SECONDS);

    assert_close(system.elapsed_seconds, SOLAR_DAY_SECONDS, 1e-9);
    assert_close(system.bodies[0].position_m.x, 0.0, 1e-12);
    assert_close(system.bodies[0].position_m.y, 0.0, 1e-12);
    assert_close(system.bodies[0].position_m.z, 0.0, 1e-12);
}

static void test_mercury_constants_are_real_si_values(void)
{
    assert_close(SOLAR_MERCURY_MASS_KG, 3.3011e23, 3.3011e23 * 1e-12);
    assert_close(SOLAR_MERCURY_RADIUS_M, 2439700.0, 1e-6);
    assert_close(SOLAR_MERCURY_SEMI_MAJOR_AXIS_M, 57909050000.0, 1e-3);
    assert_close(SOLAR_MERCURY_ECCENTRICITY, 0.205630, 1e-12);
}

static void test_mercury_perihelion_distance_is_derived_from_orbital_elements(void)
{
    double expected = SOLAR_MERCURY_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_MERCURY_ECCENTRICITY);

    assert_close(SOLAR_MERCURY_PERIHELION_M, expected, 1e-3);
}

static void test_mercury_perihelion_speed_matches_vis_viva(void)
{
    double expected = sqrt(SOLAR_G * SOLAR_SUN_MASS_KG * ((2.0 / SOLAR_MERCURY_PERIHELION_M) - (1.0 / SOLAR_MERCURY_SEMI_MAJOR_AXIS_M)));

    assert_close(SOLAR_MERCURY_PERIHELION_SPEED_MPS, expected, 1e-6);
}

static void test_solar_system_capacity_supports_sun_and_mercury(void)
{
    assert(SOLAR_SYSTEM_BODY_CAPACITY == 2);
}

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

static void test_mercury_acceleration_points_toward_sun_at_perihelion(void)
{
    SolarSystem system = solar_system_create_sun_mercury();

    solar_system_step(&system, 0.0);

    assert(system.bodies[1].acceleration_mps2.x < 0.0);
    assert_close(system.bodies[1].acceleration_mps2.y, 0.0, 1e-18);
    assert_close(system.bodies[1].acceleration_mps2.z, 0.0, 1e-18);
}

static void test_mercury_moves_after_one_day_while_sun_stays_fixed(void)
{
    SolarSystem system = solar_system_create_sun_mercury();
    Vec3d initial_mercury_position = system.bodies[1].position_m;

    solar_system_step(&system, SOLAR_DAY_SECONDS);

    assert_close(system.bodies[0].position_m.x, 0.0, 1e-12);
    assert_close(system.bodies[0].position_m.y, 0.0, 1e-12);
    assert_close(system.bodies[0].position_m.z, 0.0, 1e-12);
    assert(system.bodies[1].position_m.x < initial_mercury_position.x);
    assert(system.bodies[1].position_m.z > initial_mercury_position.z);
}

static void test_mercury_roughly_returns_after_one_orbit(void)
{
    SolarSystem system = solar_system_create_sun_mercury();
    Vec3d initial = system.bodies[1].position_m;
    const double dt_seconds = 6.0 * 60.0 * 60.0;
    const int steps = (int)((87.9677 * SOLAR_DAY_SECONDS) / dt_seconds);

    for (int i = 0; i < steps; ++i) {
        solar_system_step(&system, dt_seconds);
    }

    double distance_from_initial = vec3d_length(vec3d_sub(system.bodies[1].position_m, initial));
    assert(distance_from_initial < 0.08 * SOLAR_AU_METERS);
}

int main(void)
{
    test_sun_body_creation_preserves_fields();
    test_sun_only_system_has_one_real_sun();
    test_sun_only_step_advances_time_and_keeps_sun_fixed();
    test_mercury_constants_are_real_si_values();
    test_mercury_perihelion_distance_is_derived_from_orbital_elements();
    test_mercury_perihelion_speed_matches_vis_viva();
    test_solar_system_capacity_supports_sun_and_mercury();
    test_mercury_body_starts_at_perihelion_with_tangential_velocity();
    test_sun_mercury_system_has_two_expected_bodies();
    test_mercury_acceleration_points_toward_sun_at_perihelion();
    test_mercury_moves_after_one_day_while_sun_stays_fixed();
    test_mercury_roughly_returns_after_one_orbit();
    puts("test_solar_system passed");
    return 0;
}
