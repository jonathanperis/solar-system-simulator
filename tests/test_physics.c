#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "sim/body.h"
#include "sim/constants.h"
#include "sim/physics.h"
#include "sim/vec3d.h"

static void assert_close(double actual, double expected, double epsilon)
{
    assert(fabs(actual - expected) <= epsilon);
}

static void test_sun_only_body_acceleration_is_zero(void)
{
    Body sun = body_create("Sun", BODY_KIND_STAR, SOLAR_SUN_MASS_KG, SOLAR_SUN_RADIUS_M, vec3d_zero(), vec3d_zero(), true);

    physics_compute_accelerations(&sun, 1);

    assert_close(sun.acceleration_mps2.x, 0.0, 1e-18);
    assert_close(sun.acceleration_mps2.y, 0.0, 1e-18);
    assert_close(sun.acceleration_mps2.z, 0.0, 1e-18);
}

static void test_solar_gravity_at_one_au_has_expected_magnitude_and_direction(void)
{
    Body probe = body_create("Probe", BODY_KIND_ASTEROID, 1.0, 1.0, (Vec3d){SOLAR_AU_METERS, 0.0, 0.0}, vec3d_zero(), false);
    Body sun = body_create("Sun", BODY_KIND_STAR, SOLAR_SUN_MASS_KG, SOLAR_SUN_RADIUS_M, vec3d_zero(), vec3d_zero(), true);

    Vec3d acceleration = gravitational_acceleration_from(&probe, &sun);
    double expected = SOLAR_G * SOLAR_SUN_MASS_KG / (SOLAR_AU_METERS * SOLAR_AU_METERS);

    assert_close(acceleration.x, -expected, expected * 1e-12);
    assert_close(acceleration.y, 0.0, 1e-18);
    assert_close(acceleration.z, 0.0, 1e-18);
    assert_close(vec3d_length(acceleration), expected, expected * 1e-12);
}

static void test_zero_distance_contributes_no_acceleration(void)
{
    Body a = body_create("A", BODY_KIND_ASTEROID, 1.0, 1.0, vec3d_zero(), vec3d_zero(), false);
    Body b = body_create("B", BODY_KIND_ASTEROID, 2.0, 1.0, vec3d_zero(), vec3d_zero(), false);

    Vec3d acceleration = gravitational_acceleration_from(&a, &b);

    assert_close(acceleration.x, 0.0, 1e-18);
    assert_close(acceleration.y, 0.0, 1e-18);
    assert_close(acceleration.z, 0.0, 1e-18);
}

static void test_step_keeps_fixed_sun_at_origin(void)
{
    Body sun = body_create("Sun", BODY_KIND_STAR, SOLAR_SUN_MASS_KG, SOLAR_SUN_RADIUS_M, vec3d_zero(), (Vec3d){100.0, 0.0, 0.0}, true);

    physics_step(&sun, 1, SOLAR_DAY_SECONDS);

    assert_close(sun.position_m.x, 0.0, 1e-12);
    assert_close(sun.position_m.y, 0.0, 1e-12);
    assert_close(sun.position_m.z, 0.0, 1e-12);
    assert_close(sun.velocity_mps.x, 100.0, 1e-12);
}

static void test_step_moves_unaccelerated_body_linearly(void)
{
    Body probe = body_create("Probe", BODY_KIND_ASTEROID, 1.0, 1.0, vec3d_zero(), (Vec3d){10.0, -2.0, 0.5}, false);

    physics_step(&probe, 1, 3.0);

    assert_close(probe.position_m.x, 30.0, 1e-12);
    assert_close(probe.position_m.y, -6.0, 1e-12);
    assert_close(probe.position_m.z, 1.5, 1e-12);
    assert_close(probe.velocity_mps.x, 10.0, 1e-12);
}

int main(void)
{
    test_sun_only_body_acceleration_is_zero();
    test_solar_gravity_at_one_au_has_expected_magnitude_and_direction();
    test_zero_distance_contributes_no_acceleration();
    test_step_keeps_fixed_sun_at_origin();
    test_step_moves_unaccelerated_body_linearly();
    puts("test_physics passed");
    return 0;
}
