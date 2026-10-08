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
    /* 180 degrees reads the same in a mirrored frame, so also check a signed
     * case: Jupiter 30 degrees counterclockwise from the particle's periapsis
     * (seen from ecliptic north) gives 3*30 - 2*0 - 0 = +90 degrees. */
    SolarSystem signed_case = resonance;
    double radius_j = vec3d_length(resonance.bodies[1].position_m), speed_j = vec3d_length(resonance.bodies[1].velocity_mps);
    double lambda = acos(-1.0) / 6;
    signed_case.bodies[1].position_m = orbit_ecliptic_to_simulation((Vec3d){radius_j * cos(lambda), radius_j * sin(lambda), 0});
    signed_case.bodies[1].velocity_mps = orbit_ecliptic_to_simulation((Vec3d){-speed_j * sin(lambda), speed_j * cos(lambda), 0});
    assert(fabs(lesson_resonant_angle_degrees(&signed_case) - 90) < 1e-9);
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
    /* Pinned conic cases (r = 1e7 m; escape speed there is sqrt(2 mu / r)). */
    double r = 1e7, escape = sqrt(2 * mu / r), circular = sqrt(mu / r);
    assert(orbit_closest_approach_m((Vec3d){r, 0, 0}, (Vec3d){-1e3, 0, 0}, mu) == 0);           /* radial, falling in */
    assert(orbit_closest_approach_m((Vec3d){r, 0, 0}, (Vec3d){0.5 * escape, 0, 0}, mu) == 0);   /* radial, bound: falls back */
    assert(orbit_closest_approach_m((Vec3d){r, 0, 0}, (Vec3d){1e5, 0, 0}, mu) == r);            /* open, receding */
    assert(fabs(orbit_closest_approach_m((Vec3d){r, 0, 0}, (Vec3d){0, 0, -circular}, mu) / r - 1) < 1e-12); /* circular */
    assert(orbit_closest_approach_m(vec3d_zero(), (Vec3d){1, 0, 0}, mu) == 0);                 /* at the focus */
    /* Approaching hyperbola with impact parameter b: q = p/(1+e) from h = b v. */
    double v_in = 3 * escape, b = 2 * r, h = b * v_in;
    Vec3d start = {-10 * r, 0, b}, toward = {v_in, 0, 0};
    double energy = 0.5 * v_in * v_in - mu / vec3d_length(start);
    double e_hyp = sqrt(1 + 2 * energy * h * h / (mu * mu));
    assert(fabs(orbit_closest_approach_m(start, toward, mu) / (h * h / (mu * (1 + e_hyp))) - 1) < 1e-12);
    /* Near-parabolic, tangential at periapsis: the start itself is closest. */
    assert(fabs(orbit_closest_approach_m((Vec3d){r, 0, 0}, (Vec3d){0, 0, -escape * (1 - 1e-12)}, mu) / r - 1) < 1e-9);
    assert(fabs(orbit_closest_approach_m((Vec3d){r, 0, 0}, (Vec3d){0, 0, -escape * (1 + 1e-12)}, mu) / r - 1) < 1e-9);
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
    /* Aim Phobos so one 15 s step carries it from one side of Mars to the
     * other: it crosses the centre mid-step, yet both sampled endpoints lie
     * outside the contact sphere, so only the swept test can notice. */
    Body *moon = &session.system.bodies[1];
    const Body *mars = &session.system.bodies[0];
    Vec3d across = vec3d_scale(vec3d_sub(mars->position_m, moon->position_m), 2.0 / 15.0);
    moon->velocity_mps = vec3d_add(mars->velocity_mps, across);
    simulation_session_single_step(&session);
    double contact = SOLAR_MARS_RADIUS_M + SOLAR_PHOBOS_RADIUS_M;
    assert(vec3d_length(vec3d_sub(session.system.bodies[1].position_m, session.system.bodies[0].position_m)) > contact);
    assert(session.clock.contact_tick == 1);
    simulation_session_single_step(&session);
    assert(session.clock.contact_tick == 1);
    simulation_session_reset(&session);
    assert(session.clock.contact_tick == 0 && session.clock.monitor_contact);
    assert(simulation_session_start_lesson(&session, LESSON_CORE, 1, PHYSICS_VERLET, 15));
    assert(!session.clock.monitor_contact);
    /* Catalog experiments (up to 25 bodies) are monitored too; the swept
     * check sizes its buffer to the full scene capacity. */
    assert(simulation_session_start_experiment(&session, "SOLAR_EXPERIMENT_V1 2461200.5\n"
        "20000004\tVesta\t2.148\t0.09\t7.14\t103.7\t151.4\t2461000.5\t0\t0\t2\t2\n"));
    assert(session.clock.monitor_contact && session.clock.contact_tick == 0);
    Body *vesta = &session.system.bodies[9];
    vesta->velocity_mps = vec3d_scale(vec3d_sub(session.system.bodies[0].position_m, vesta->position_m), 2.0 / 15.0);
    simulation_session_single_step(&session);
    assert(session.clock.contact_tick == 1);
    simulation_session_destroy(&session);
}

static double two_body_period_s(const SolarSystem *lesson)
{
    SimulationSession session = simulation_session_create();
    session.system = *lesson;
    BodyInspection body = simulation_session_inspect_body(&session, 1);
    simulation_session_destroy(&session);
    return body.orbital_period_s;
}

/* SPEC A96: DART slowed Dimorphos along its orbit; 0.985 of the pre-impact
 * speed shortens the two-body period by about half an hour (observed ~33 min). */
static void test_dart_lesson_reproduces_the_period_change(void)
{
    SolarSystem before, after;
    assert(lesson_create(LESSON_DART, 1, &before) && lesson_create(LESSON_DART, 0.985, &after));
    assert(before.body_count == 2 && before.bodies[0].id == BODY_ID_DIDYMOS && before.bodies[1].id == BODY_ID_DIMORPHOS);
    double pre = two_body_period_s(&before), post = two_body_period_s(&after);
    assert(fabs(pre / 44418.722 - 1) < 0.01);
    double minutes = (pre - post) / 60;
    assert(minutes > 28 && minutes < 36);
    /* Started at periapsis, so the analytic two-body reference applies. */
    Vec3d relative;
    assert(lesson_reference_position(&before, 1, 3600, &relative));
    /* The orbit is retrograde seen from ecliptic north (i ~ 171 deg). */
    Vec3d h = vec3d_cross(vec3d_sub(before.bodies[1].position_m, before.bodies[0].position_m),
        vec3d_sub(before.bodies[1].velocity_mps, before.bodies[0].velocity_mps));
    assert(h.y < 0);
}

/* SPEC A96: Charon is 12% of Pluto's mass, so the pair's barycenter (the
 * lesson origin) lies outside Pluto. */
static void test_pluto_charon_lesson_orbits_a_point_outside_pluto(void)
{
    SolarSystem pair;
    assert(lesson_create(LESSON_PLUTO_CHARON, 1, &pair));
    assert(pair.body_count == 2 && pair.bodies[0].id == BODY_ID_PLUTO && pair.bodies[1].id == BODY_ID_CHARON);
    assert(vec3d_length(pair.bodies[0].position_m) > SOLAR_PLUTO_RADIUS_M);
    Vec3d momentum = vec3d_add(vec3d_scale(pair.bodies[0].velocity_mps, pair.bodies[0].mass_kg),
        vec3d_scale(pair.bodies[1].velocity_mps, pair.bodies[1].mass_kg));
    assert(vec3d_length(momentum) < 1e-6 * pair.bodies[1].mass_kg);
    assert(fabs(two_body_period_s(&pair) / (6.387222 * SOLAR_DAY_SECONDS) - 1) < 0.002);
    Vec3d relative;
    assert(lesson_reference_position(&pair, 1, 86400, &relative));
}

int main(void)
{
    test_lessons_refuse_speeds_whose_orbit_enters_the_parent();
    test_dart_lesson_reproduces_the_period_change();
    test_pluto_charon_lesson_orbits_a_point_outside_pluto();
    test_contact_is_detected_along_each_step();
    test_barycentric_translation_and_force_decomposition();
    test_resonance_and_encounter_states_are_explicit_experiments();
    test_head_on_collision_conservation_and_reset();
    puts("test_advanced_lessons passed");
    return 0;
}
