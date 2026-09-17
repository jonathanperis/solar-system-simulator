#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "app/simulation_session.h"
#include "sim/collisions.h"
#include "sim/constants.h"

static void test_barycentric_translation_and_force_decomposition(void)
{
    SolarSystem core = solar_system_create_current(), barycentric;
    assert(lesson_create(LESSON_BARYCENTRIC_CORE, 1, &barycentric));
    PhysicsDiagnostics d = physics_diagnostics(&barycentric);
    assert(d.isolated && vec3d_length(d.center_of_mass_m) < 1e-5);
    double momentum_scale = SOLAR_JUPITER_MASS_KG * 15000;
    assert(vec3d_length(d.momentum_kg_mps) / momentum_scale < 1e-14);
    for (size_t i = 1; i < core.body_count; ++i) {
        Vec3d a = vec3d_sub(core.bodies[i].position_m, core.bodies[0].position_m);
        Vec3d b = vec3d_sub(barycentric.bodies[i].position_m, barycentric.bodies[0].position_m);
        assert(vec3d_length(vec3d_sub(a, b)) / vec3d_length(a) < 1e-14);
    }
    Vec3d sun = barycentric.bodies[0].position_m;
    for (int i = 0; i < 100; ++i) solar_system_step(&barycentric, 15);
    assert(vec3d_length(vec3d_sub(sun, barycentric.bodies[0].position_m)) > 1);
    assert(vec3d_length(vec3d_sub(physics_diagnostics(&barycentric).momentum_kg_mps, d.momentum_kg_mps)) / momentum_scale < 1e-12);
    ForceContribution forces[SOLAR_SYSTEM_BODY_CAPACITY];
    size_t count = physics_force_breakdown(&core, 4, forces, SOLAR_SYSTEM_BODY_CAPACITY);
    Vec3d sum = vec3d_zero(); double fractions = 0;
    for (size_t i = 0; i < count; ++i) {
        sum = vec3d_add(sum, forces[i].acceleration_mps2);
        fractions += forces[i].magnitude_fraction;
        if (i) assert(forces[i-1].magnitude_mps2 >= forces[i].magnitude_mps2);
    }
    physics_compute_accelerations(core.bodies, core.body_count);
    assert(vec3d_length(vec3d_sub(sum, core.bodies[4].acceleration_mps2)) < 1e-15);
    assert(fabs(fractions - 1) < 1e-14);
}

static void test_resonance_and_encounter_states_are_explicit_experiments(void)
{
    SolarSystem resonance, encounter;
    assert(lesson_create(LESSON_RESONANCE, 1, &resonance));
    assert(lesson_create(LESSON_ENCOUNTER, 1, &encounter));
    assert(resonance.body_count == 3 && resonance.bodies[2].mass_kg == 0);
    double mu = SOLAR_G * SOLAR_SUN_MASS_KG;
    double q = vec3d_length(resonance.bodies[2].position_m);
    double axis = 1 / (2 / q - vec3d_length_squared(resonance.bodies[2].velocity_mps) / mu);
    assert(fabs(pow(axis / vec3d_length(resonance.bodies[1].position_m), 1.5) - 2.0 / 3.0) < 1e-12);
    assert(fabs(fabs(lesson_resonant_angle_degrees(&resonance)) - 180) < 1e-9);
    assert(encounter.bodies[2].parent_id == BODY_ID_EARTH);
    double radius = vec3d_length(vec3d_sub(encounter.bodies[2].position_m, encounter.bodies[1].position_m));
    assert(radius > 40 * SOLAR_EARTH_RADIUS_M && radius < 41 * SOLAR_EARTH_RADIUS_M);
}

static void test_head_on_collision_conservation_and_reset(void)
{
    for (int mode = COLLISION_BOUNCE; mode <= COLLISION_MERGE; ++mode) {
        SimulationSession session = simulation_session_create();
        assert(simulation_session_start_configured_lesson(&session, LESSON_COLLISION, 1, PHYSICS_VERLET, .1, (CollisionMode)mode));
        assert(simulation_session_time_scale(&session) == 5);
        PhysicsDiagnostics before = physics_diagnostics(&session.system);
        session.paused = true;
        for (int i = 0; i < 200; ++i) simulation_session_single_step(&session);
        PhysicsDiagnostics after = physics_diagnostics(&session.system);
        assert(session.clock.collision_count == 1);
        assert(after.total_mass_kg == before.total_mass_kg);
        assert(vec3d_length(vec3d_sub(after.momentum_kg_mps, before.momentum_kg_mps)) < 1e-10);
        if (mode == COLLISION_BOUNCE) {
            assert(session.system.body_count == 2);
            assert(session.system.bodies[0].velocity_mps.x < 0);
            assert(fabs(after.kinetic_energy_j / before.kinetic_energy_j - 1) < 1e-9);
        } else {
            assert(session.system.body_count == 1 && session.selected_body_index == 0);
            assert(after.kinetic_energy_j < 1e-10);
            assert(fabs(session.system.bodies[0].radius_m - cbrt(2000)) < 1e-12);
            assert(session.clock.dissipated_energy_j > 999);
        }
        simulation_session_reset(&session);
        assert(session.system.body_count == 2 && session.clock.collision_count == 0);
        assert(!simulation_session_start_configured_lesson(&session, LESSON_COLLISION, 1, PHYSICS_VERLET, 15, (CollisionMode)mode));
        simulation_session_destroy(&session);
    }
}

int main(void)
{
    test_barycentric_translation_and_force_decomposition();
    test_resonance_and_encounter_states_are_explicit_experiments();
    test_head_on_collision_conservation_and_reset();
    puts("test_advanced_lessons passed");
    return 0;
}
