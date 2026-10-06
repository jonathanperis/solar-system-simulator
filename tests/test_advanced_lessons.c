#include "require_assert.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "app/simulation_session.h"
#include "sim/collisions.h"
#include "sim/constants.h"
#include "sim/orbit.h"

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

/* Point-mass gravity has no surface: below these factors the analytic
 * two-body periapsis lies inside the parent plus subject radius (A54). */
static void test_lessons_refuse_speeds_whose_orbit_enters_the_parent(void)
{
    const struct { LessonPreset preset; double low, high, former_failure; } limits[] = {
        {LESSON_PHOBOS, 0.72, 0.74, 0.5},      /* 1,343 km periapsis vs 3,390 km Mars */
        {LESSON_EARTH_MOON, 0.20, 0.21, 0.15}, /* Earth + Moon radii */
        {LESSON_ECCENTRIC, 0.11, 0.12, 0.1},   /* Sun + Earth radii from 0.5 AU */
        {LESSON_ENCOUNTER, 0.28, 0.31, 0.25},  /* hyperbolic flyby, 4 Earth radii offset */
    };
    for (size_t k = 0; k < sizeof(limits) / sizeof(limits[0]); ++k) {
        double minimum = lesson_minimum_velocity_factor(limits[k].preset);
        assert(minimum >= limits[k].low && minimum <= limits[k].high);
        assert(fabs(minimum * 100 - round(minimum * 100)) < 1e-9); /* whole hundredths for UI steps */
        SolarSystem system;
        assert(!lesson_create(limits[k].preset, limits[k].former_failure, &system));
        assert(!lesson_create(limits[k].preset, minimum - 0.01, &system));
        assert(!lesson_configuration_valid(limits[k].preset, limits[k].former_failure, PHYSICS_VERLET, 15, COLLISION_NONE));
        assert(lesson_create(limits[k].preset, minimum, &system));
        assert(lesson_configuration_valid(limits[k].preset, minimum, PHYSICS_VERLET, 15, COLLISION_NONE));
        size_t subject = lesson_subject_index(limits[k].preset);
        int parent = solar_system_parent_index(&system, subject);
        assert(parent >= 0);
        const Body *body = &system.bodies[subject], *center = &system.bodies[parent];
        double mu = SOLAR_G * (center->mass_kg + (center->fixed ? 0 : body->mass_kg));
        double closest = orbit_closest_approach_m(vec3d_sub(body->position_m, center->position_m),
            vec3d_sub(body->velocity_mps, center->velocity_mps), mu);
        assert(closest >= center->radius_m + body->radius_m);
    }
    /* Lessons that never reach contact keep the general 0.1 floor; fixed
     * presets keep factor 1; the collision lesson models contact itself. */
    assert(lesson_minimum_velocity_factor(LESSON_CIRCULAR) == 0.1);
    assert(lesson_minimum_velocity_factor(LESSON_ESCAPE) == 0.1);
    assert(lesson_minimum_velocity_factor(LESSON_INCLINED) == 0.1);
    assert(lesson_minimum_velocity_factor(LESSON_RESONANCE) == 0.1);
    assert(lesson_minimum_velocity_factor(LESSON_COLLISION) == 0.1);
    assert(lesson_minimum_velocity_factor(LESSON_CORE) == 1.0);
    assert(lesson_minimum_velocity_factor(LESSON_BARYCENTRIC_CORE) == 1.0);

    /* Independent closed form for one case: from apoapsis r0 with tangential
     * speed v, the other apsis is r0 k / (2 - k) with k = v^2 r0 / mu. */
    double r0 = SOLAR_PHOBOS_PERIAREION_M, mu = SOLAR_G * (SOLAR_MARS_MASS_KG + SOLAR_PHOBOS_MASS_KG);
    double v = 0.5 * SOLAR_PHOBOS_PERIAREION_SPEED_MPS, kk = v * v * r0 / mu;
    double expected = r0 * kk / (2 - kk);
    assert(fabs(orbit_closest_approach_m((Vec3d){r0, 0, 0}, (Vec3d){0, 0, -v}, mu) / expected - 1) < 1e-12);
    assert(expected < 1.4e6 && expected > 1.3e6); /* the audit's 1,343 km */
    /* An open orbit already moving away has no future periapsis. */
    assert(orbit_closest_approach_m((Vec3d){1e7, 0, 0}, (Vec3d){1e5, 0, 0}, mu) == 1e7);
}

static void test_contact_is_detected_along_each_step(void)
{
    SolarSystem system = {.body_count = 2};
    system.bodies[0] = body_create("A", BODY_KIND_PLANET, 1, 10, vec3d_zero(), vec3d_zero(), false);
    system.bodies[1] = body_create("B", BODY_KIND_ASTEROID, 1, 0, (Vec3d){-100, 1, 0}, vec3d_zero(), false);
    Vec3d before[2] = {system.bodies[0].position_m, system.bodies[1].position_m};
    /* The endpoints are 100 m apart on either side, but the straight path
     * between them passes 1 m from A's center: a 10 m sphere was crossed. */
    system.bodies[1].position_m = (Vec3d){100, 1, 0};
    assert(collision_contact_during_step(before, &system));
    system.bodies[1].position_m = (Vec3d){100, 20, 0};
    before[1] = (Vec3d){-100, 20, 0};
    assert(!collision_contact_during_step(before, &system));

    /* A session flags the first contact tick and keeps it until reset. */
    SimulationSession session = simulation_session_create();
    double minimum = lesson_minimum_velocity_factor(LESSON_PHOBOS);
    assert(simulation_session_start_lesson(&session, LESSON_PHOBOS, minimum, PHYSICS_VERLET, 15));
    assert(session.clock.monitor_contact && session.clock.contact_tick == 0);
    session.paused = true;
    Body *moon = &session.system.bodies[1];
    Vec3d inward = vec3d_scale(vec3d_sub(session.system.bodies[0].position_m, moon->position_m), 1.0 / 15.0);
    moon->velocity_mps = vec3d_add(session.system.bodies[0].velocity_mps, inward);
    simulation_session_single_step(&session);
    assert(session.clock.contact_tick == 1);
    simulation_session_single_step(&session);
    assert(session.clock.contact_tick == 1);
    simulation_session_reset(&session);
    assert(session.clock.contact_tick == 0 && session.clock.monitor_contact);
    assert(simulation_session_start_lesson(&session, LESSON_CORE, 1, PHYSICS_VERLET, 15));
    assert(!session.clock.monitor_contact);
    simulation_session_destroy(&session);
}

int main(void)
{
    test_lessons_refuse_speeds_whose_orbit_enters_the_parent();
    test_contact_is_detected_along_each_step();
    test_barycentric_translation_and_force_decomposition();
    test_resonance_and_encounter_states_are_explicit_experiments();
    test_head_on_collision_conservation_and_reset();
    puts("test_advanced_lessons passed");
    return 0;
}
