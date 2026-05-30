#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "app/body_trails.h"
#include "render/renderer.h"
#include "sim/constants.h"
#include "sim/solar_system.h"
#include "sim/units.h"

static void assert_close(double actual, double expected, double epsilon)
{
    assert(fabs(actual - expected) <= epsilon);
}

static void test_real_scale_radius_uses_physical_meter_scale(void)
{
    Body earth = solar_system_create_earth_at_perihelion();
    Body moon = solar_system_create_moon_at_perigee_near_earth(&earth);

    assert_close(renderer_body_radius(&earth, RENDER_SCALE_REAL), meters_to_render_units(SOLAR_EARTH_RADIUS_M), 1e-12);
    assert_close(renderer_body_radius(&moon, RENDER_SCALE_REAL), meters_to_render_units(SOLAR_MOON_RADIUS_M), 1e-12);
}

static void test_real_scale_position_uses_physical_meter_scale(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon();
    Vec3d earth_position = renderer_body_position(&system, 3, RENDER_SCALE_REAL);
    Vec3d moon_position = renderer_body_position(&system, 4, RENDER_SCALE_REAL);
    Vec3d expected_earth = meters_vec_to_render_vec3d(system.bodies[3].position_m);
    Vec3d expected_moon = meters_vec_to_render_vec3d(system.bodies[4].position_m);

    assert_close(earth_position.x, expected_earth.x, 1e-12);
    assert_close(earth_position.y, expected_earth.y, 1e-12);
    assert_close(earth_position.z, expected_earth.z, 1e-12);
    assert_close(moon_position.x, expected_moon.x, 1e-12);
    assert_close(moon_position.y, expected_moon.y, 1e-12);
    assert_close(moon_position.z, expected_moon.z, 1e-12);
}

static void test_illustrative_planets_keep_old_visible_radius(void)
{
    Body earth = solar_system_create_earth_at_perihelion();
    Body mars = solar_system_create_mars_at_perihelion();

    assert_close(renderer_body_radius(&earth, RENDER_SCALE_ILLUSTRATIVE), SOLAR_ILLUSTRATIVE_PLANET_RADIUS, 1e-6);
    assert_close(renderer_body_radius(&mars, RENDER_SCALE_ILLUSTRATIVE), SOLAR_ILLUSTRATIVE_PLANET_RADIUS, 1e-6);
}

static void test_real_scale_mars_radius_uses_physical_meter_scale(void)
{
    Body mars = solar_system_create_mars_at_perihelion();

    assert_close(renderer_body_radius(&mars, RENDER_SCALE_REAL), meters_to_render_units(SOLAR_MARS_RADIUS_M), 1e-12);
}

static void test_illustrative_moon_is_smaller_but_still_visible(void)
{
    Body earth = solar_system_create_earth_at_perihelion();
    Body moon = solar_system_create_moon_at_perigee_near_earth(&earth);

    float earth_radius = renderer_body_radius(&earth, RENDER_SCALE_ILLUSTRATIVE);
    float moon_radius = renderer_body_radius(&moon, RENDER_SCALE_ILLUSTRATIVE);
    float expected_moon_radius = SOLAR_ILLUSTRATIVE_PLANET_RADIUS * (float)(SOLAR_MOON_RADIUS_M / SOLAR_EARTH_RADIUS_M);

    assert_close(moon_radius, expected_moon_radius, 1e-6);
    assert(moon_radius > meters_to_render_units(SOLAR_MOON_RADIUS_M));
    assert(earth_radius > moon_radius);
}

static void test_illustrative_earth_and_moon_do_not_overlap_at_perigee(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon();
    Vec3d earth_position = renderer_body_position(&system, 3, RENDER_SCALE_ILLUSTRATIVE);
    Vec3d moon_position = renderer_body_position(&system, 4, RENDER_SCALE_ILLUSTRATIVE);
    double render_distance = vec3d_length(vec3d_sub(moon_position, earth_position));

    double combined_radius =
        renderer_body_radius(&system.bodies[3], RENDER_SCALE_ILLUSTRATIVE) +
        renderer_body_radius(&system.bodies[4], RENDER_SCALE_ILLUSTRATIVE);

    assert(combined_radius < render_distance);
}

static void test_real_scale_trail_point_uses_physical_meter_scale(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon();
    BodyTrails trails = body_trails_create();
    body_trails_record_system(&trails, &system);

    Vec3d actual = renderer_trail_point_position(&system, &trails, 3, 0, RENDER_SCALE_REAL);
    Vec3d expected = meters_vec_to_render_vec3d(system.bodies[3].position_m);

    assert_close(actual.x, expected.x, 1e-12);
    assert_close(actual.y, expected.y, 1e-12);
    assert_close(actual.z, expected.z, 1e-12);
}

static void test_illustrative_moon_trail_uses_expanded_parent_relative_position(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon();
    BodyTrails trails = body_trails_create();
    body_trails_record_system(&trails, &system);

    Vec3d actual = renderer_trail_point_position(&system, &trails, 4, 0, RENDER_SCALE_ILLUSTRATIVE);
    Vec3d earth = meters_vec_to_render_vec3d(system.bodies[3].position_m);
    Vec3d moon = meters_vec_to_render_vec3d(system.bodies[4].position_m);
    Vec3d expected = vec3d_add(earth, vec3d_scale(vec3d_sub(moon, earth), SOLAR_ILLUSTRATIVE_MOON_DISTANCE_FACTOR));

    assert_close(actual.x, expected.x, 1e-12);
    assert_close(actual.y, expected.y, 1e-12);
    assert_close(actual.z, expected.z, 1e-12);
}

int main(void)
{
    test_real_scale_radius_uses_physical_meter_scale();
    test_real_scale_position_uses_physical_meter_scale();
    test_illustrative_planets_keep_old_visible_radius();
    test_real_scale_mars_radius_uses_physical_meter_scale();
    test_illustrative_moon_is_smaller_but_still_visible();
    test_illustrative_earth_and_moon_do_not_overlap_at_perigee();
    test_real_scale_trail_point_uses_physical_meter_scale();
    test_illustrative_moon_trail_uses_expanded_parent_relative_position();
    puts("test_renderer passed");
    return 0;
}
