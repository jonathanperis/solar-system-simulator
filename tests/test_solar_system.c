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
    double expected = sqrt(SOLAR_G * SOLAR_SUN_MASS_KG * ((2.0 / SOLAR_VENUS_PERIHELION_M) - (1.0 / SOLAR_VENUS_SEMI_MAJOR_AXIS_M)));

    assert_close(SOLAR_VENUS_PERIHELION_SPEED_MPS, expected, 1e-6);
}

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
    double expected = sqrt(SOLAR_G * SOLAR_SUN_MASS_KG * ((2.0 / SOLAR_EARTH_PERIHELION_M) - (1.0 / SOLAR_EARTH_SEMI_MAJOR_AXIS_M)));

    assert_close(SOLAR_EARTH_PERIHELION_SPEED_MPS, expected, 1e-6);
}

static void test_moon_constants_are_real_si_values(void)
{
    assert_close(SOLAR_MOON_MASS_KG, 7.346e22, 7.346e22 * 1e-12);
    assert_close(SOLAR_MOON_RADIUS_M, 1737400.0, 1e-6);
    assert_close(SOLAR_MOON_SEMI_MAJOR_AXIS_M, 384400000.0, 1e-6);
    assert_close(SOLAR_MOON_ECCENTRICITY, 0.0549, 1e-12);
}

static void test_moon_perigee_distance_is_derived_from_orbital_elements(void)
{
    double expected = SOLAR_MOON_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_MOON_ECCENTRICITY);

    assert_close(SOLAR_MOON_PERIGEE_M, expected, 1e-6);
}

static void test_moon_perigee_speed_matches_earth_moon_vis_viva(void)
{
    double mu = SOLAR_G * (SOLAR_EARTH_MASS_KG + SOLAR_MOON_MASS_KG);
    double expected = sqrt(mu * ((2.0 / SOLAR_MOON_PERIGEE_M) - (1.0 / SOLAR_MOON_SEMI_MAJOR_AXIS_M)));

    assert_close(SOLAR_MOON_PERIGEE_SPEED_MPS, expected, 1e-9);
}

static void test_mars_constants_are_real_si_values(void)
{
    assert_close(SOLAR_MARS_MASS_KG, 6.419e23, 6.419e23 * 1e-12);
    assert_close(SOLAR_MARS_RADIUS_M, 3390000.0, 1e-6);
    assert_close(SOLAR_MARS_SEMI_MAJOR_AXIS_M, 227900000000.0, 1e-3);
    assert_close(SOLAR_MARS_ECCENTRICITY, 0.0934, 1e-12);
}

static void test_mars_perihelion_distance_is_derived_from_orbital_elements(void)
{
    double expected = SOLAR_MARS_SEMI_MAJOR_AXIS_M * (1.0 - SOLAR_MARS_ECCENTRICITY);

    assert_close(SOLAR_MARS_PERIHELION_M, expected, 1e-3);
}

static void test_mars_aphelion_distance_is_derived_from_orbital_elements(void)
{
    double expected = SOLAR_MARS_SEMI_MAJOR_AXIS_M * (1.0 + SOLAR_MARS_ECCENTRICITY);

    assert_close(SOLAR_MARS_APHELION_M, expected, 1e-3);
}

static void test_mars_perihelion_speed_matches_vis_viva(void)
{
    double expected = sqrt(SOLAR_G * SOLAR_SUN_MASS_KG * ((2.0 / SOLAR_MARS_PERIHELION_M) - (1.0 / SOLAR_MARS_SEMI_MAJOR_AXIS_M)));

    assert_close(SOLAR_MARS_PERIHELION_SPEED_MPS, expected, 1e-6);
}

static void test_solar_system_capacity_supports_sun_mercury_venus_earth_moon_and_mars(void)
{
    assert(SOLAR_SYSTEM_BODY_CAPACITY == 6);
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
    assert_close(earth.velocity_mps.x, -SOLAR_EARTH_PERIHELION_SPEED_MPS, 1e-6);
    assert_close(earth.velocity_mps.y, 0.0, 1e-12);
    assert_close(earth.velocity_mps.z, 0.0, 1e-12);
}

static void test_moon_body_starts_at_earth_relative_perigee_with_tangential_velocity(void)
{
    Body earth = solar_system_create_earth_at_perihelion();
    Body moon = solar_system_create_moon_at_perigee_near_earth(&earth);
    Vec3d earth_to_moon = vec3d_sub(moon.position_m, earth.position_m);
    Vec3d relative_velocity = vec3d_sub(moon.velocity_mps, earth.velocity_mps);

    assert(strcmp(moon.name, "Moon") == 0);
    assert(moon.kind == BODY_KIND_MOON);
    assert(!moon.fixed);
    assert_close(moon.mass_kg, SOLAR_MOON_MASS_KG, SOLAR_MOON_MASS_KG * 1e-12);
    assert_close(moon.radius_m, SOLAR_MOON_RADIUS_M, 1e-6);
    assert_close(earth_to_moon.x, SOLAR_MOON_PERIGEE_M, 1e-6);
    assert_close(earth_to_moon.y, 0.0, 1e-12);
    assert_close(earth_to_moon.z, 0.0, 1e-12);
    assert_close(relative_velocity.x, 0.0, 1e-12);
    assert_close(relative_velocity.y, 0.0, 1e-12);
    assert_close(relative_velocity.z, SOLAR_MOON_PERIGEE_SPEED_MPS, 1e-9);
}

static void test_mars_body_starts_at_perihelion_with_tangential_velocity(void)
{
    Body mars = solar_system_create_mars_at_perihelion();

    assert(strcmp(mars.name, "Mars") == 0);
    assert(mars.kind == BODY_KIND_PLANET);
    assert(!mars.fixed);
    assert_close(mars.mass_kg, SOLAR_MARS_MASS_KG, SOLAR_MARS_MASS_KG * 1e-12);
    assert_close(mars.radius_m, SOLAR_MARS_RADIUS_M, 1e-6);
    assert_close(mars.position_m.x, 0.0, 1e-12);
    assert_close(mars.position_m.y, 0.0, 1e-12);
    assert_close(mars.position_m.z, -SOLAR_MARS_PERIHELION_M, 1e-3);
    assert_close(mars.velocity_mps.x, SOLAR_MARS_PERIHELION_SPEED_MPS, 1e-6);
    assert_close(mars.velocity_mps.y, 0.0, 1e-12);
    assert_close(mars.velocity_mps.z, 0.0, 1e-12);
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

static void test_sun_mercury_venus_earth_moon_system_has_five_expected_bodies(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon();

    assert(system.body_count == 5);
    assert(strcmp(system.bodies[0].name, "Sun") == 0);
    assert(strcmp(system.bodies[1].name, "Mercury") == 0);
    assert(strcmp(system.bodies[2].name, "Venus") == 0);
    assert(strcmp(system.bodies[3].name, "Earth") == 0);
    assert(strcmp(system.bodies[4].name, "Moon") == 0);
    assert(system.bodies[0].fixed);
    assert(!system.bodies[4].fixed);
    assert(system.bodies[4].kind == BODY_KIND_MOON);
}

static void test_sun_mercury_venus_earth_moon_mars_system_has_six_expected_bodies(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars();

    assert(system.body_count == 6);
    assert(strcmp(system.bodies[0].name, "Sun") == 0);
    assert(strcmp(system.bodies[1].name, "Mercury") == 0);
    assert(strcmp(system.bodies[2].name, "Venus") == 0);
    assert(strcmp(system.bodies[3].name, "Earth") == 0);
    assert(strcmp(system.bodies[4].name, "Moon") == 0);
    assert(strcmp(system.bodies[5].name, "Mars") == 0);
    assert(system.bodies[0].fixed);
    assert(!system.bodies[5].fixed);
    assert(system.bodies[5].kind == BODY_KIND_PLANET);
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

static void test_venus_acceleration_points_toward_sun_at_perihelion(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus();

    solar_system_step(&system, 0.0);

    assert(system.bodies[2].acceleration_mps2.x > 0.0);
    assert_close(system.bodies[2].acceleration_mps2.y, 0.0, 1e-18);
    assert_close(system.bodies[2].acceleration_mps2.z, 0.0, 1e-18);
}

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

static void test_earth_acceleration_points_toward_sun_at_perihelion(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth();

    solar_system_step(&system, 0.0);

    assert(system.bodies[3].acceleration_mps2.z < 0.0);
    assert_close(system.bodies[3].acceleration_mps2.y, 0.0, 1e-18);
    assert(fabs(system.bodies[3].acceleration_mps2.z) > fabs(system.bodies[3].acceleration_mps2.x));
}

static void test_earth_moves_after_one_day_while_sun_stays_fixed(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth();
    Vec3d initial_earth_position = system.bodies[3].position_m;

    solar_system_step(&system, SOLAR_DAY_SECONDS);

    assert_close(system.bodies[0].position_m.x, 0.0, 1e-12);
    assert_close(system.bodies[0].position_m.y, 0.0, 1e-12);
    assert_close(system.bodies[0].position_m.z, 0.0, 1e-12);
    assert(system.bodies[3].position_m.x < initial_earth_position.x);
    assert(system.bodies[3].position_m.z < initial_earth_position.z);
}

static void test_earth_roughly_returns_after_one_orbit(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth();
    Vec3d initial = system.bodies[3].position_m;
    const double dt_seconds = SOLAR_DAY_SECONDS;
    const int steps = (int)((365.2514 * SOLAR_DAY_SECONDS) / dt_seconds);

    for (int i = 0; i < steps; ++i) {
        solar_system_step(&system, dt_seconds);
    }

    double distance_from_initial = vec3d_length(vec3d_sub(system.bodies[3].position_m, initial));
    assert(distance_from_initial < 0.08 * SOLAR_AU_METERS);
}

static void test_moon_moves_prograde_relative_to_earth_after_one_day(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon();
    Vec3d initial_relative = vec3d_sub(system.bodies[4].position_m, system.bodies[3].position_m);

    solar_system_step(&system, SOLAR_DAY_SECONDS);

    Vec3d relative = vec3d_sub(system.bodies[4].position_m, system.bodies[3].position_m);
    assert(relative.x < initial_relative.x);
    assert(relative.z > initial_relative.z);
    assert_close(system.bodies[0].position_m.x, 0.0, 1e-12);
    assert_close(system.bodies[0].position_m.y, 0.0, 1e-12);
    assert_close(system.bodies[0].position_m.z, 0.0, 1e-12);
}

static void test_mars_acceleration_points_toward_sun_at_perihelion(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars();

    solar_system_step(&system, 0.0);

    assert(system.bodies[5].acceleration_mps2.z > 0.0);
    assert_close(system.bodies[5].acceleration_mps2.y, 0.0, 1e-18);
    assert(fabs(system.bodies[5].acceleration_mps2.z) > fabs(system.bodies[5].acceleration_mps2.x));
}

static void test_mars_moves_after_one_day_while_sun_stays_fixed(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars();
    Vec3d initial_mars_position = system.bodies[5].position_m;

    solar_system_step(&system, SOLAR_DAY_SECONDS);

    assert_close(system.bodies[0].position_m.x, 0.0, 1e-12);
    assert_close(system.bodies[0].position_m.y, 0.0, 1e-12);
    assert_close(system.bodies[0].position_m.z, 0.0, 1e-12);
    assert(system.bodies[5].position_m.x > initial_mars_position.x);
    assert(system.bodies[5].position_m.z > initial_mars_position.z);
}

static void test_mars_roughly_returns_after_one_orbit(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars();
    Vec3d initial = system.bodies[5].position_m;
    const double dt_seconds = SOLAR_DAY_SECONDS;
    const double mars_period_seconds = 2.0 * acos(-1.0) * sqrt(
        (SOLAR_MARS_SEMI_MAJOR_AXIS_M * SOLAR_MARS_SEMI_MAJOR_AXIS_M * SOLAR_MARS_SEMI_MAJOR_AXIS_M) /
        (SOLAR_G * SOLAR_SUN_MASS_KG)
    );
    const int steps = (int)(mars_period_seconds / dt_seconds);

    for (int i = 0; i < steps; ++i) {
        solar_system_step(&system, dt_seconds);
    }

    double distance_from_initial = vec3d_length(vec3d_sub(system.bodies[5].position_m, initial));
    assert(distance_from_initial < 0.12 * SOLAR_AU_METERS);
}

static void test_moon_remains_near_earth_after_one_lunar_orbit(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon();
    const double dt_seconds = 3.0 * 60.0 * 60.0;
    const double earth_moon_mu = SOLAR_G * (SOLAR_EARTH_MASS_KG + SOLAR_MOON_MASS_KG);
    const double pi = acos(-1.0);
    const double lunar_period_seconds = 2.0 * pi * sqrt(
        (SOLAR_MOON_SEMI_MAJOR_AXIS_M * SOLAR_MOON_SEMI_MAJOR_AXIS_M * SOLAR_MOON_SEMI_MAJOR_AXIS_M) /
        earth_moon_mu
    );
    const int steps = (int)(lunar_period_seconds / dt_seconds);

    for (int i = 0; i < steps; ++i) {
        solar_system_step(&system, dt_seconds);
    }

    Vec3d earth_to_moon = vec3d_sub(system.bodies[4].position_m, system.bodies[3].position_m);
    double distance = vec3d_length(earth_to_moon);

    assert(distance > 0.80 * SOLAR_MOON_PERIGEE_M);
    assert(distance < 1.30 * (SOLAR_MOON_SEMI_MAJOR_AXIS_M * (1.0 + SOLAR_MOON_ECCENTRICITY)));
}

int main(void)
{
    test_sun_body_creation_preserves_fields();
    test_sun_only_system_has_one_real_sun();
    test_sun_only_step_advances_time_and_keeps_sun_fixed();
    test_mercury_constants_are_real_si_values();
    test_mercury_perihelion_distance_is_derived_from_orbital_elements();
    test_mercury_perihelion_speed_matches_vis_viva();
    test_venus_constants_are_real_si_values();
    test_venus_perihelion_distance_is_derived_from_orbital_elements();
    test_venus_perihelion_speed_matches_vis_viva();
    test_earth_constants_are_real_si_values();
    test_earth_perihelion_distance_is_derived_from_orbital_elements();
    test_earth_perihelion_speed_matches_vis_viva();
    test_moon_constants_are_real_si_values();
    test_moon_perigee_distance_is_derived_from_orbital_elements();
    test_moon_perigee_speed_matches_earth_moon_vis_viva();
    test_mars_constants_are_real_si_values();
    test_mars_perihelion_distance_is_derived_from_orbital_elements();
    test_mars_aphelion_distance_is_derived_from_orbital_elements();
    test_mars_perihelion_speed_matches_vis_viva();
    test_solar_system_capacity_supports_sun_mercury_venus_earth_moon_and_mars();
    test_mercury_body_starts_at_perihelion_with_tangential_velocity();
    test_venus_body_starts_at_perihelion_with_tangential_velocity();
    test_earth_body_starts_at_perihelion_with_tangential_velocity();
    test_moon_body_starts_at_earth_relative_perigee_with_tangential_velocity();
    test_mars_body_starts_at_perihelion_with_tangential_velocity();
    test_sun_mercury_system_has_two_expected_bodies();
    test_sun_mercury_venus_system_has_three_expected_bodies();
    test_sun_mercury_venus_earth_system_has_four_expected_bodies();
    test_sun_mercury_venus_earth_moon_system_has_five_expected_bodies();
    test_sun_mercury_venus_earth_moon_mars_system_has_six_expected_bodies();
    test_mercury_acceleration_points_toward_sun_at_perihelion();
    test_mercury_moves_after_one_day_while_sun_stays_fixed();
    test_mercury_roughly_returns_after_one_orbit();
    test_venus_acceleration_points_toward_sun_at_perihelion();
    test_venus_moves_after_one_day_while_sun_stays_fixed();
    test_venus_roughly_returns_after_one_orbit();
    test_earth_acceleration_points_toward_sun_at_perihelion();
    test_earth_moves_after_one_day_while_sun_stays_fixed();
    test_earth_roughly_returns_after_one_orbit();
    test_moon_moves_prograde_relative_to_earth_after_one_day();
    test_mars_acceleration_points_toward_sun_at_perihelion();
    test_mars_moves_after_one_day_while_sun_stays_fixed();
    test_mars_roughly_returns_after_one_orbit();
    test_moon_remains_near_earth_after_one_lunar_orbit();
    puts("test_solar_system passed");
    return 0;
}
