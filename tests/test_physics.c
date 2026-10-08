#include "require_assert.h"

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

static void test_massless_particles_feel_gravity_without_backreaction(void)
{
    Body bodies[] = {
        body_create("Source", BODY_KIND_PLANET, 1e20, 100, vec3d_zero(), vec3d_zero(), false),
        body_create("Tracer", BODY_KIND_MOON, 0, 0, (Vec3d){1e6, 0, 0}, vec3d_zero(), false),
    };
    physics_step(bodies, 2, 15);
    assert(bodies[0].position_m.x == 0 && bodies[0].velocity_mps.x == 0);
    assert(bodies[1].position_m.x < 1e6 && bodies[1].velocity_mps.x < 0);
}

static void test_optimized_accelerations_match_pairwise_formula(void)
{
    Body bodies[] = {
        body_create("A", BODY_KIND_PLANET, 1e20, 100, (Vec3d){0, 0, 0}, vec3d_zero(), false),
        body_create("Tracer", BODY_KIND_MOON, 0, 0, (Vec3d){1e6, -2e6, 3e6}, vec3d_zero(), false),
        body_create("B", BODY_KIND_MOON, 2e18, 10, (Vec3d){-3e6, 1e6, 2e6}, vec3d_zero(), false)
    };
    Vec3d expected[3] = {0};
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 3; ++j) expected[i] = vec3d_add(expected[i], gravitational_acceleration_from(&bodies[i], &bodies[j]));
    }
    physics_compute_accelerations(bodies, 3);
    for (size_t i = 0; i < 3; ++i) {
        assert_close(bodies[i].acceleration_mps2.x, expected[i].x, 1e-18);
        assert_close(bodies[i].acceleration_mps2.y, expected[i].y, 1e-18);
        assert_close(bodies[i].acceleration_mps2.z, expected[i].z, 1e-18);
    }
}

/* A94: a test particle around an oblate planet regresses its node at the
 * secular J2 rate -3/2 n J2 (R/a)^2 cos i. */
static void test_j2_nodal_precession_matches_the_secular_rate(void)
{
    const double radius = 60268000.0, j2 = 0.016298, mass = 5.6832e26;
    const double a = 3.0 * radius, inclination = acos(-1.0) / 6.0;
    Body planet = body_create_identified("Planet", BODY_KIND_PLANET, (BodyId)1, BODY_ID_NONE, mass, radius,
        vec3d_zero(), vec3d_zero(), true);
    planet.j2 = j2;
    planet.j2_radius_m = radius;
    planet.pole = (Vec3d){0, 1, 0};
    double mu = SOLAR_G * mass, speed = sqrt(mu / a);
    /* Circular orbit through +X, tilted about X by the inclination: the
     * ascending node starts on +X in the equatorial (x, z) plane. */
    Body moon = body_create_identified("Moon", BODY_KIND_MOON, (BodyId)2, (BodyId)1, 0, 0, (Vec3d){a, 0, 0},
        (Vec3d){0, speed * sin(inclination), -speed * cos(inclination)}, false);
    Body bodies[] = {planet, moon};
    const double dt = 15, days = 20;
    double first = 0, previous = 0, unwrapped = 0;
    for (int day = 0; day <= days; ++day) {
        Vec3d h = vec3d_cross(bodies[1].position_m, bodies[1].velocity_mps);
        /* Node line = pole x h, in the equatorial plane; its angle about +Y. */
        Vec3d node = vec3d_cross((Vec3d){0, 1, 0}, h);
        double angle = atan2(-node.z, node.x);
        if (day == 0) first = previous = unwrapped = angle;
        else {
            double step = atan2(sin(angle - previous), cos(angle - previous));
            unwrapped += step;
            previous = angle;
        }
        for (int i = 0; i < 5760 && day < days; ++i) physics_step(bodies, 2, dt);
    }
    double measured = (unwrapped - first) / (days * SOLAR_DAY_SECONDS);
    double n = sqrt(mu / (a * a * a));
    double expected = -1.5 * n * j2 * (radius / a) * (radius / a) * cos(inclination);
    assert(measured < 0 && fabs(measured / expected - 1) < 0.02);
}

/* A94: the planet takes the reaction to its J2 pull on a massive moon. */
static void test_j2_conserves_momentum(void)
{
    Body planet = body_create_identified("Planet", BODY_KIND_PLANET, (BodyId)1, BODY_ID_NONE, 1.9e27, 7e7,
        vec3d_zero(), vec3d_zero(), false);
    planet.j2 = 0.0147;
    planet.j2_radius_m = 7.1492e7;
    planet.pole = (Vec3d){0, 1, 0};
    Body moon = body_create_identified("Moon", BODY_KIND_MOON, (BodyId)2, (BodyId)1, 1.5e23, 2.6e6,
        (Vec3d){1.07e9, 2e7, 0}, (Vec3d){0, 300, 10880}, false);
    Body bodies[] = {planet, moon};
    Vec3d before = vec3d_add(vec3d_scale(bodies[0].velocity_mps, bodies[0].mass_kg), vec3d_scale(bodies[1].velocity_mps, bodies[1].mass_kg));
    for (int i = 0; i < 5760; ++i) physics_step(bodies, 2, 15);
    Vec3d after = vec3d_add(vec3d_scale(bodies[0].velocity_mps, bodies[0].mass_kg), vec3d_scale(bodies[1].velocity_mps, bodies[1].mass_kg));
    double scale = bodies[1].mass_kg * 10880;
    assert(vec3d_length(vec3d_sub(after, before)) < 1e-9 * scale);
}

int main(void)
{
    test_optimized_accelerations_match_pairwise_formula();
    test_massless_particles_feel_gravity_without_backreaction();
    test_j2_nodal_precession_matches_the_secular_rate();
    test_j2_conserves_momentum();
    test_sun_only_body_acceleration_is_zero();
    test_solar_gravity_at_one_au_has_expected_magnitude_and_direction();
    test_zero_distance_contributes_no_acceleration();
    test_step_keeps_fixed_sun_at_origin();
    test_step_moves_unaccelerated_body_linearly();
    puts("test_physics passed");
    return 0;
}
