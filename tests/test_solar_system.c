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

int main(void)
{
    test_sun_body_creation_preserves_fields();
    test_sun_only_system_has_one_real_sun();
    test_sun_only_step_advances_time_and_keeps_sun_fixed();
    puts("test_solar_system passed");
    return 0;
}
