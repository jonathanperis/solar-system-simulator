#include <assert.h>
#include <math.h>
#include <stdio.h>

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

static void test_illustrative_radius_preserves_parent_satellite_size_order(void)
{
    Body earth = solar_system_create_earth_at_perihelion();
    Body moon = solar_system_create_moon_at_perigee_near_earth(&earth);

    float earth_radius = renderer_body_radius(&earth, RENDER_SCALE_ILLUSTRATIVE);
    float moon_radius = renderer_body_radius(&moon, RENDER_SCALE_ILLUSTRATIVE);

    assert(moon_radius > meters_to_render_units(SOLAR_MOON_RADIUS_M));
    assert(earth_radius > moon_radius);
}

static void test_illustrative_earth_and_moon_do_not_overlap_at_perigee(void)
{
    Body earth = solar_system_create_earth_at_perihelion();
    Body moon = solar_system_create_moon_at_perigee_near_earth(&earth);
    Vec3d earth_to_moon = vec3d_sub(moon.position_m, earth.position_m);
    double render_distance = vec3d_length(meters_vec_to_render_vec3d(earth_to_moon));

    double combined_radius =
        renderer_body_radius(&earth, RENDER_SCALE_ILLUSTRATIVE) +
        renderer_body_radius(&moon, RENDER_SCALE_ILLUSTRATIVE);

    assert(combined_radius < render_distance);
}

int main(void)
{
    test_real_scale_radius_uses_physical_meter_scale();
    test_illustrative_radius_preserves_parent_satellite_size_order();
    test_illustrative_earth_and_moon_do_not_overlap_at_perigee();
    puts("test_renderer passed");
    return 0;
}
