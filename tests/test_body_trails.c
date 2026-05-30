#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app/body_trails.h"
#include "sim/constants.h"
#include "sim/solar_system.h"

static void assert_vec3d_equal(Vec3d actual, Vec3d expected)
{
    assert(actual.x == expected.x);
    assert(actual.y == expected.y);
    assert(actual.z == expected.z);
}

static void test_trails_start_empty_for_all_body_slots(void)
{
    BodyTrails trails = body_trails_create();

    for (size_t i = 0; i < SOLAR_SYSTEM_BODY_CAPACITY; ++i) {
        assert(body_trails_point_count(&trails, i) == 0);
    }
}

static void test_trails_record_planets_and_moons_but_not_stars(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon();
    BodyTrails trails = body_trails_create();

    body_trails_record_system(&trails, &system);

    assert(body_trails_point_count(&trails, 0) == 0);
    assert(body_trails_point_count(&trails, 1) == 1);
    assert(body_trails_point_count(&trails, 2) == 1);
    assert(body_trails_point_count(&trails, 3) == 1);
    assert(body_trails_point_count(&trails, 4) == 1);
    assert_vec3d_equal(body_trails_point_at(&trails, 3, 0), system.bodies[3].position_m);
    assert_vec3d_equal(body_trails_point_at(&trails, 4, 0), system.bodies[4].position_m);
}

static void test_trails_append_new_positions_after_motion(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon();
    BodyTrails trails = body_trails_create();

    body_trails_record_system(&trails, &system);
    solar_system_step(&system, SOLAR_DAY_SECONDS);
    body_trails_record_system(&trails, &system);

    assert(body_trails_point_count(&trails, 3) == 2);
    assert(body_trails_point_count(&trails, 4) == 2);
    assert_vec3d_equal(body_trails_point_at(&trails, 3, 1), system.bodies[3].position_m);
    assert_vec3d_equal(body_trails_point_at(&trails, 4, 1), system.bodies[4].position_m);
}

static void test_trails_wrap_at_capacity_and_keep_newest_points(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon();
    BodyTrails trails = body_trails_create();

    for (size_t i = 0; i < SOLAR_TRAIL_POINT_CAPACITY + 3; ++i) {
        system.bodies[3].position_m = (Vec3d){(double)i, 0.0, 0.0};
        body_trails_record_system(&trails, &system);
    }

    assert(body_trails_point_count(&trails, 3) == SOLAR_TRAIL_POINT_CAPACITY);
    assert_vec3d_equal(body_trails_point_at(&trails, 3, 0), (Vec3d){3.0, 0.0, 0.0});
    assert_vec3d_equal(body_trails_point_at(&trails, 3, SOLAR_TRAIL_POINT_CAPACITY - 1), (Vec3d){(double)(SOLAR_TRAIL_POINT_CAPACITY + 2), 0.0, 0.0});
}

int main(void)
{
    test_trails_start_empty_for_all_body_slots();
    test_trails_record_planets_and_moons_but_not_stars();
    test_trails_append_new_positions_after_motion();
    test_trails_wrap_at_capacity_and_keep_newest_points();
    puts("test_body_trails passed");
    return 0;
}
