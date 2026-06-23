#include <assert.h>
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

static void test_long_frame_records_trails_for_each_physics_substep(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos();
    BodyTrails trails = body_trails_create();

    body_trails_record_system(&trails, &system);
    solar_app_step_system_with_trails(&system, &trails, SOLAR_DAY_SECONDS, SOLAR_APP_MAX_PHYSICS_STEP_SECONDS);

    assert(body_trails_point_count(&trails, 0) == 0);
    assert(body_trails_point_count(&trails, 1) == 1 + 288);
    assert(body_trails_point_count(&trails, 3) == 1 + 288);
    assert(body_trails_point_count(&trails, 5) == 1 + 288);
    assert(body_trails_point_count(&trails, 6) == 1 + 288);
    assert(body_trails_point_count(&trails, 7) == 1 + 288);
    assert_vec3d_equal(body_trails_point_at(&trails, 6, body_trails_point_count(&trails, 6) - 1), system.bodies[6].position_m);
    assert_vec3d_equal(body_trails_point_at(&trails, 7, body_trails_point_count(&trails, 7) - 1), system.bodies[7].position_m);

    body_trails_destroy(&trails);
}

static void test_partial_frame_records_remainder_step(void)
{
    SolarSystem system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos();
    BodyTrails trails = body_trails_create();

    body_trails_record_system(&trails, &system);
    solar_app_step_system_with_trails(&system, &trails, 650.0, 300.0);

    assert(body_trails_point_count(&trails, 1) == 4);
    assert_vec3d_equal(body_trails_point_at(&trails, 1, 3), system.bodies[1].position_m);

    body_trails_destroy(&trails);
}

int main(void)
{
    test_long_frame_records_trails_for_each_physics_substep();
    test_partial_frame_records_remainder_step();
    puts("test_simulation_step passed");
    return 0;
}
