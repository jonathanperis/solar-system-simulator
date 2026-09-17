#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "sim/diagnostics.h"
#include "sim/lessons.h"
#include "sim/physics.h"
#include "sim/constants.h"

static void test_lesson_states_and_physical_diagnostics(void)
{
    SolarSystem circular, escape, pair, tilted;
    assert(lesson_create(LESSON_CIRCULAR, 1.0, &circular));
    assert(lesson_create(LESSON_ESCAPE, 1.0, &escape));
    assert(lesson_create(LESSON_EARTH_MOON, 1.0, &pair));
    assert(lesson_create(LESSON_INCLINED, 1.0, &tilted));
    PhysicsDiagnostics d = physics_diagnostics(&circular);
    double expected = -SOLAR_G * SOLAR_SUN_MASS_KG * SOLAR_EARTH_MASS_KG / (2 * SOLAR_AU_METERS);
    assert(fabs((d.total_energy_j - expected) / expected) < 1e-14);
    assert(!d.isolated);
    assert(fabs(physics_diagnostics(&escape).total_energy_j / expected) < 1e-14);
    assert(physics_diagnostics(&pair).isolated);
    assert(vec3d_length(physics_diagnostics(&pair).center_of_mass_m) < 1e-8);
    assert(fabs(tilted.bodies[1].velocity_mps.y) > 1000);
    SolarSystem before = pair;
    assert(!lesson_create(LESSON_CIRCULAR, NAN, &pair));
    assert(!lesson_create(LESSON_CIRCULAR, 0.0, &pair));
    assert(memcmp(&before, &pair, sizeof(pair)) == 0);

    /* A free two-body system has no external constraint force. Its momentum
     * and angular momentum, unlike a fixed-Sun scene's momentum, are conserved. */
    PhysicsDiagnostics initial = physics_diagnostics(&pair);
    for (int i = 0; i < 10000; ++i) solar_system_step(&pair, 15.0);
    PhysicsDiagnostics final = physics_diagnostics(&pair);
    double scale = pair.bodies[1].mass_kg * SOLAR_MOON_PERIGEE_SPEED_MPS;
    assert(vec3d_length(vec3d_sub(final.momentum_kg_mps, initial.momentum_kg_mps)) / scale < 1e-12);
    assert(vec3d_length(vec3d_sub(final.angular_momentum_kg_m2ps, initial.angular_momentum_kg_m2ps)) /
           vec3d_length(initial.angular_momentum_kg_m2ps) < 1e-12);
}

static double circular_error(int steps, PhysicsIntegrator method, double *energy_error)
{
    SolarSystem system;
    assert(lesson_create(LESSON_CIRCULAR, 1.0, &system));
    double initial_energy = physics_diagnostics(&system).total_energy_j;
    double omega = sqrt(SOLAR_G * SOLAR_SUN_MASS_KG / pow(SOLAR_AU_METERS, 3));
    double duration = 0.25 * 2.0 * acos(-1.0) / omega;
    for (int i = 0; i < steps; ++i) physics_step_with_integrator(system.bodies, system.body_count, duration / steps, method);
    Vec3d expected = {0, 0, SOLAR_AU_METERS};
    *energy_error = fabs((physics_diagnostics(&system).total_energy_j - initial_energy) / initial_energy);
    return vec3d_length(vec3d_sub(system.bodies[1].position_m, expected)) / SOLAR_AU_METERS;
}

static void test_verlet_convergence_and_euler_comparison(void)
{
    double coarse_energy, fine_energy, euler_energy;
    double coarse = circular_error(100, PHYSICS_VERLET, &coarse_energy);
    double fine = circular_error(200, PHYSICS_VERLET, &fine_energy);
    double euler = circular_error(100, PHYSICS_EULER, &euler_energy);
    assert(coarse / fine > 3.9 && coarse / fine < 4.1);
    assert(fine_energy < coarse_energy && coarse_energy < 1e-7);
    assert(euler > coarse * 10 && euler_energy > coarse_energy * 100);
    printf("Verlet step-halving position ratio %.6f; Euler energy error %.6g\n", coarse / fine, euler_energy);
}

int main(void)
{
    test_lesson_states_and_physical_diagnostics();
    test_verlet_convergence_and_euler_comparison();
    puts("test_learning_lab passed");
    return 0;
}
