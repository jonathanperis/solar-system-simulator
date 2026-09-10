#include "simulation_session.h"

#include <ctype.h>

#include "../sim/constants.h"

SimulationSession simulation_session_create(void)
{
    SimulationSession session = {.trails = body_trails_create(), .speed_preset = 1};
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
    session->system = solar_system_create_current();
    session->clock = (SimulationClock){0};
    session->achieved_time_scale = 0;
    session->rate_real_seconds = 0;
    session->rate_sim_seconds = 0;
    body_trails_record_system(&session->trails, &session->system);
}

double simulation_session_time_scale(const SimulationSession *session)
{
    const double rates[SOLAR_SPEED_PRESET_COUNT] = {3600.0, SOLAR_DAY_SECONDS, 5.0 * SOLAR_DAY_SECONDS,
        10.0 * SOLAR_DAY_SECONDS, 15.0 * SOLAR_DAY_SECONDS};
    return rates[session->speed_preset];
}

void simulation_session_update(SimulationSession *session, double real_seconds)
{
    if (!session->paused) {
        double before = session->system.elapsed_seconds;
        solar_app_step_system_with_trails(&session->system, &session->trails, &session->clock,
            real_seconds * simulation_session_time_scale(session), SOLAR_APP_MAX_STEPS_PER_UPDATE);
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
        solar_system_step(&session->system, SOLAR_APP_MAX_PHYSICS_STEP_SECONDS);
        body_trails_record_system(&session->trails, &session->system);
    }
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
        result.distance_m = vec3d_length(vec3d_sub(body->position_m, parent->position_m));
        result.speed_mps = vec3d_length(vec3d_sub(body->velocity_mps, parent->velocity_mps));
    }
    return result;
}
