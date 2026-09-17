#include "simulation_session.h"

#include <ctype.h>
#include <string.h>

#include "../sim/constants.h"

SimulationSession simulation_session_create(void)
{
    SimulationSession session = {.trails = body_trails_create(), .speed_preset = 1, .velocity_factor = 1.0};
    session.initial_system = solar_system_create_current();
    simulation_session_reset(&session);
    return session;
}

void simulation_session_destroy(SimulationSession *session)
{
    body_trails_destroy(&session->trails);
}

void simulation_session_reset(SimulationSession *session)
{
    /* Reset the physical experiment, retaining the observer's control settings. */
    body_trails_destroy(&session->trails);
    session->system = session->initial_system;
    session->clock = (SimulationClock){.step_seconds = simulation_clock_step_seconds(&session->clock),
        .integrator = session->clock.integrator, .collision_mode = session->clock.collision_mode};
    /* Lesson steps need not divide 300 seconds. Align the initial cadence to
     * whole ticks, so later compaction doubles both actual and reported spacing. */
    session->trails.sample_interval_seconds = ceil(SOLAR_TRAIL_INITIAL_INTERVAL_SECONDS / session->clock.step_seconds)
        * session->clock.step_seconds;
    if (session->lesson == LESSON_COLLISION) session->trails.sample_interval_seconds = session->clock.step_seconds;
    physics_compute_accelerations(session->system.bodies, session->system.body_count);
    PhysicsDiagnostics initial = physics_diagnostics(&session->system);
    session->initial_energy_j = initial.total_energy_j;
    session->energy_scale_j = initial.kinetic_energy_j + fabs(initial.potential_energy_j);
    session->achieved_time_scale = 0;
    session->rate_real_seconds = 0;
    session->rate_sim_seconds = 0;
    body_trails_record_system(&session->trails, &session->system);
}

bool simulation_session_start_lesson(SimulationSession *session, LessonPreset lesson, double velocity_factor,
    PhysicsIntegrator integrator, double step_seconds)
{
    return simulation_session_start_configured_lesson(session, lesson, velocity_factor, integrator, step_seconds,
        lesson == LESSON_COLLISION ? COLLISION_BOUNCE : COLLISION_NONE);
}

bool lesson_configuration_valid(LessonPreset lesson, double velocity_factor, PhysicsIntegrator integrator,
    double step_seconds, CollisionMode collision_mode)
{
    if (lesson < 0 || lesson >= LESSON_COUNT || !isfinite(velocity_factor) || velocity_factor < .1 || velocity_factor > 2 ||
        !isfinite(step_seconds) || step_seconds < .01 || step_seconds > 3600 ||
        (integrator != PHYSICS_VERLET && integrator != PHYSICS_EULER) ||
        (lesson == LESSON_CORE && (integrator != PHYSICS_VERLET || step_seconds != SOLAR_APP_MAX_PHYSICS_STEP_SECONDS)) ||
        ((lesson == LESSON_CORE || lesson == LESSON_BARYCENTRIC_CORE) && velocity_factor != 1)) return false;
    /* At the largest allowed speed, a .25 s drift is only 10 m: smaller than
     * the 20 m contact separation. These head-on spheres cannot tunnel. */
    return lesson == LESSON_COLLISION ? step_seconds <= .25 &&
        (collision_mode == COLLISION_BOUNCE || collision_mode == COLLISION_MERGE) : collision_mode == COLLISION_NONE;
}

bool simulation_session_start_configured_lesson(SimulationSession *session, LessonPreset lesson, double velocity_factor,
    PhysicsIntegrator integrator, double step_seconds, CollisionMode collision_mode)
{
    if (!lesson_configuration_valid(lesson, velocity_factor, integrator, step_seconds, collision_mode)) return false;
    SolarSystem initial;
    if (!lesson_create(lesson, velocity_factor, &initial)) return false;
    session->initial_system = initial;
    session->catalog_experiment = false;
    session->lesson = lesson;
    session->velocity_factor = velocity_factor;
    session->selected_body_index = lesson_subject_index(lesson);
    session->clock.step_seconds = step_seconds;
    session->clock.integrator = integrator;
    session->clock.collision_mode = collision_mode;
    simulation_session_reset(session);
    return true;
}

void simulation_session_set_background(SimulationSession *session, bool hidden)
{
    if (session->background && !hidden) session->discard_resumed_frame = true;
    session->background = hidden;
}

double simulation_session_energy_change(const SimulationSession *session, const PhysicsDiagnostics *diagnostics)
{
    /* K0 + |U0| stays well-conditioned at the escape threshold where E0 is zero. */
    return session->energy_scale_j > 0 ? (diagnostics->total_energy_j - session->initial_energy_j) / session->energy_scale_j : 0;
}

bool simulation_session_start_experiment(SimulationSession *session, const char *text)
{
    SolarSystem trial;
    char names[SOLAR_EXPERIMENT_CAPACITY][SOLAR_EXPERIMENT_NAME_BYTES];
    if (!experiment_parse(text, &trial, names)) return false;
    /* Commit only after every row succeeds. Names belong to the session, not
     * the temporary parser or the browser's short-lived UTF-8 input buffer. */
    for (size_t i = 9; i < trial.body_count; ++i) {
        strcpy(session->imported_names[i-9], names[i-9]);
        trial.bodies[i].name = session->imported_names[i-9];
    }
    session->initial_system = trial;
    session->catalog_experiment = true;
    session->lesson = LESSON_CATALOG;
    session->velocity_factor = 1.0;
    session->clock = (SimulationClock){.step_seconds = SOLAR_APP_MAX_PHYSICS_STEP_SECONDS};
    session->selected_body_index = 9;
    simulation_session_reset(session);
    return true;
}

void simulation_session_demo(SimulationSession *session)
{
    simulation_session_start_lesson(session, LESSON_CORE, 1, PHYSICS_VERLET, SOLAR_APP_MAX_PHYSICS_STEP_SECONDS);
}

double simulation_session_time_scale(const SimulationSession *session)
{
    const double contact_rates[SOLAR_SPEED_PRESET_COUNT] = {1, 5, 10, 25, 50};
    if (session->lesson == LESSON_COLLISION) return contact_rates[session->speed_preset];
    const double rates[SOLAR_SPEED_PRESET_COUNT] = {3600.0, SOLAR_DAY_SECONDS, 5.0 * SOLAR_DAY_SECONDS,
        10.0 * SOLAR_DAY_SECONDS, 15.0 * SOLAR_DAY_SECONDS};
    return rates[session->speed_preset];
}

void simulation_session_update(SimulationSession *session, double real_seconds)
{
    if (session->background) return;
    if (session->discard_resumed_frame) {
        session->discard_resumed_frame = false;
        return;
    }
    if (!session->paused) {
        double before = session->system.elapsed_seconds;
        solar_app_step_system_with_trails(&session->system, &session->trails, &session->clock,
            real_seconds * simulation_session_time_scale(session), SOLAR_APP_MAX_STEPS_PER_UPDATE);
        if (session->selected_body_index >= session->system.body_count) session->selected_body_index = 0;
        session->rate_sim_seconds += session->system.elapsed_seconds - before;
        session->rate_real_seconds += real_seconds;
        if (session->rate_real_seconds >= 0.5) {
            session->achieved_time_scale = session->rate_sim_seconds / session->rate_real_seconds;
            session->rate_real_seconds = 0;
            session->rate_sim_seconds = 0;
        }
    }
}

void simulation_session_single_step(SimulationSession *session)
{
    if (session->paused) {
        /* A manual step leaves the fractional playback remainder untouched. */
        physics_compute_accelerations(session->system.bodies, session->system.body_count);
        simulation_session_advance_tick(session, true);
    }
}

void simulation_session_advance_tick(SimulationSession *session, bool record_history)
{
    simulation_clock_tick(&session->system, record_history ? &session->trails : NULL, &session->clock);
    if (session->selected_body_index >= session->system.body_count) session->selected_body_index = 0;
}

void simulation_session_set_speed(SimulationSession *session, int preset)
{
    if (preset >= 0 && preset < SOLAR_SPEED_PRESET_COUNT) session->speed_preset = preset;
}

void simulation_session_select_body(SimulationSession *session, int index)
{
    if (index >= 0 && (size_t)index < session->system.body_count) session->selected_body_index = (size_t)index;
}

static bool contains_query(const char *name, const char *query)
{
    for (; *name; ++name) {
        size_t i = 0;
        while (query[i] && name[i] && tolower((unsigned char)query[i]) == tolower((unsigned char)name[i])) ++i;
        if (!query[i]) return true;
    }
    return false;
}

int simulation_session_find_body(const SimulationSession *session, const char *query, size_t start)
{
    for (size_t offset = 0; offset < session->system.body_count; ++offset) {
        size_t index = (start + offset) % session->system.body_count;
        const Body *body = &session->system.bodies[index];
        if (contains_query(body->name, query) || (body->group && contains_query(body->group, query))) return (int)index;
    }
    return -1;
}

BodyInspection simulation_session_inspect(const SimulationSession *session)
{
    const Body *body = &session->system.bodies[session->selected_body_index];
    int parent_index = solar_system_parent_index(&session->system, session->selected_body_index);
    BodyInspection result = {.name = body->name, .parent_name = "None", .has_parent = parent_index >= 0,
        .mass_kg = body->mass_kg, .radius_m = body->radius_m,
        .mass_quality = body->mass_quality, .radius_quality = body->radius_quality};
    if (result.has_parent) {
        const Body *parent = &session->system.bodies[parent_index];
        result.parent_name = parent->name;
        /* Subtract both absolute states: a moon's heliocentric speed is not its
         * speed around the parent. Rendering scale never enters this calculation. */
        result.relative_position_m = vec3d_sub(body->position_m, parent->position_m);
        result.relative_velocity_mps = vec3d_sub(body->velocity_mps, parent->velocity_mps);
        result.relative_acceleration_mps2 = vec3d_sub(body->acceleration_mps2,
            parent->fixed ? vec3d_zero() : parent->acceleration_mps2);
        result.distance_m = vec3d_length(result.relative_position_m);
        result.speed_mps = vec3d_length(result.relative_velocity_mps);
        result.acceleration_mps2 = vec3d_length(result.relative_acceleration_mps2);
        double mu = SOLAR_G * (parent->mass_kg + (parent->fixed ? 0 : body->mass_kg));
        if (result.distance_m > 0) result.specific_energy_jpkg = 0.5 * result.speed_mps * result.speed_mps - mu / result.distance_m;
    }
    return result;
}
