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


static void test_illustrative_martian_moons_are_visible_and_outside_mars(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos();
    Vec3d mars_position = renderer_body_position(&system, 5, RENDER_SCALE_ILLUSTRATIVE);
    Vec3d phobos_position = renderer_body_position(&system, 6, RENDER_SCALE_ILLUSTRATIVE);
    Vec3d deimos_position = renderer_body_position(&system, 7, RENDER_SCALE_ILLUSTRATIVE);
    double phobos_distance = vec3d_length(vec3d_sub(phobos_position, mars_position));
    double deimos_distance = vec3d_length(vec3d_sub(deimos_position, mars_position));
    double mars_radius = renderer_body_radius(&system.bodies[5], RENDER_SCALE_ILLUSTRATIVE);
    double phobos_radius = renderer_body_radius(&system.bodies[6], RENDER_SCALE_ILLUSTRATIVE);
    double deimos_radius = renderer_body_radius(&system.bodies[7], RENDER_SCALE_ILLUSTRATIVE);

    assert(phobos_radius > 0.0);
    assert(deimos_radius > 0.0);
    assert(phobos_radius < mars_radius);
    assert(deimos_radius < mars_radius);
    assert(mars_radius + phobos_radius < phobos_distance);
    assert(mars_radius + deimos_radius < deimos_distance);
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


static void test_illustrative_phobos_trail_matches_visible_body_position(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos();
    BodyTrails trails = body_trails_create();
    body_trails_record_system(&trails, &system);

    Vec3d body_position = renderer_body_position(&system, 6, RENDER_SCALE_ILLUSTRATIVE);
    Vec3d trail_position = renderer_trail_point_position(&system, &trails, 6, 0, RENDER_SCALE_ILLUSTRATIVE);

    assert_close(trail_position.x, body_position.x, 1e-12);
    assert_close(trail_position.y, body_position.y, 1e-12);
    assert_close(trail_position.z, body_position.z, 1e-12);
}

static void test_illustrative_satellite_position_uses_parent_metadata_not_name(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos();
    system.bodies[6].name = "Inner Mars Moon";

    Vec3d mars_position = renderer_body_position(&system, 5, RENDER_SCALE_ILLUSTRATIVE);
    Vec3d phobos_position = renderer_body_position(&system, 6, RENDER_SCALE_ILLUSTRATIVE);
    double render_distance = vec3d_length(vec3d_sub(phobos_position, mars_position));
    double combined_radius =
        renderer_body_radius(&system.bodies[5], RENDER_SCALE_ILLUSTRATIVE) +
        renderer_body_radius(&system.bodies[6], RENDER_SCALE_ILLUSTRATIVE);

    assert(combined_radius < render_distance);
}

static void test_body_color_uses_catalog_id_not_name(void)
{
    Body mars = solar_system_create_mars_at_perihelion();
    mars.name = "Red Planet";

    Color color = renderer_body_color(&mars);

    assert(color.r == ORANGE.r);
    assert(color.g == ORANGE.g);
    assert(color.b == ORANGE.b);
    assert(color.a == ORANGE.a);
}

static void test_body_style_adds_visual_identity_without_name_lookup(void)
{
    Body earth = solar_system_create_earth_at_perihelion();
    Body sun = solar_system_create_sun_only().bodies[0];
    earth.name = "Blue Marble";

    RendererBodyStyle earth_style = renderer_body_style(&earth);
    RendererBodyStyle sun_style = renderer_body_style(&sun);

    assert(earth_style.base.r != earth_style.accent.r || earth_style.base.g != earth_style.accent.g || earth_style.base.b != earth_style.accent.b);
    assert(sun_style.base.r >= sun_style.rim.r);
    assert(sun_style.base.g > sun_style.rim.g);
    assert(sun_style.glow.a > earth_style.glow.a);
}

static void test_trail_alpha_fades_from_history_to_current_position(void)
{
    float early_alpha = renderer_trail_segment_alpha(0, 12);
    float late_alpha = renderer_trail_segment_alpha(11, 12);

    assert(early_alpha >= 0.10f);
    assert(late_alpha <= 0.82f);
    assert(late_alpha > early_alpha);
    assert_close(renderer_trail_segment_alpha(0, 1), renderer_trail_segment_alpha(8, 0), 1e-6);
}

static void test_starfield_samples_are_bounded_and_twinkle_only_in_alpha(void)
{
    RendererStar star_a = renderer_starfield_star(17, 0.0);
    RendererStar star_b = renderer_starfield_star(17, 90.0);

    assert(star_a.x_norm >= 0.0f && star_a.x_norm <= 1.0f);
    assert(star_a.y_norm >= 0.0f && star_a.y_norm <= 1.0f);
    assert(star_a.radius >= 0.6f && star_a.radius <= 2.1f);
    assert(star_a.alpha >= 0.15f && star_a.alpha <= 0.95f);
    assert(star_b.alpha >= 0.15f && star_b.alpha <= 0.95f);
    assert_close(star_a.x_norm, star_b.x_norm, 1e-6);
    assert_close(star_a.y_norm, star_b.y_norm, 1e-6);
}

static void test_grid_keeps_at_least_minimum_square_count_for_inner_system(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon();
    int slices = renderer_grid_slices_for_system(&system, RENDER_SCALE_ILLUSTRATIVE);

    assert(slices >= 20);
    assert((slices % 2) == 0);
}

static void test_grid_expands_to_cover_mars_orbit_with_padding(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars();
    Vec3d mars_position = renderer_body_position(&system, 5, RENDER_SCALE_ILLUSTRATIVE);
    double required_half_width = fabs(mars_position.z) + 2.0;
    int slices = renderer_grid_slices_for_system(&system, RENDER_SCALE_ILLUSTRATIVE);

    assert(slices > 20);
    assert(((double)slices / 2.0) >= required_half_width);
}

static void test_trail_rendering_keeps_long_runs_bounded(void)
{
    size_t points_after_500_days = 1 + (size_t)500 * 288;
    size_t rendered_segments = renderer_trail_draw_segment_count(points_after_500_days);

    assert(rendered_segments <= SOLAR_RENDER_MAX_TRAIL_SEGMENTS);
    assert(renderer_trail_sample_stride(points_after_500_days) > 1);
    assert(renderer_trail_draw_segment_count(2) == 1);
}

int main(void)
{
    test_real_scale_radius_uses_physical_meter_scale();
    test_real_scale_position_uses_physical_meter_scale();
    test_illustrative_planets_keep_old_visible_radius();
    test_real_scale_mars_radius_uses_physical_meter_scale();
    test_illustrative_moon_is_smaller_but_still_visible();
    test_illustrative_earth_and_moon_do_not_overlap_at_perigee();
    test_illustrative_martian_moons_are_visible_and_outside_mars();
    test_real_scale_trail_point_uses_physical_meter_scale();
    test_illustrative_moon_trail_uses_expanded_parent_relative_position();
    test_illustrative_phobos_trail_matches_visible_body_position();
    test_illustrative_satellite_position_uses_parent_metadata_not_name();
    test_body_color_uses_catalog_id_not_name();
    test_body_style_adds_visual_identity_without_name_lookup();
    test_trail_alpha_fades_from_history_to_current_position();
    test_starfield_samples_are_bounded_and_twinkle_only_in_alpha();
    test_grid_keeps_at_least_minimum_square_count_for_inner_system();
    test_grid_expands_to_cover_mars_orbit_with_padding();
    test_trail_rendering_keeps_long_runs_bounded();
    puts("test_renderer passed");
    return 0;
}
