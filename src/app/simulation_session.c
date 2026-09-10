#include "simulation_session.h"

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
    session->system = solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos_vesta_jupiter();
    session->clock = (SimulationClock){0};
    body_trails_record_system(&session->trails, &session->system);
}

double simulation_session_time_scale(const SimulationSession *session)
{
    const double rates[SOLAR_SPEED_PRESET_COUNT] = {3600.0, SOLAR_DAY_SECONDS, 5.0 * SOLAR_DAY_SECONDS};
    return rates[session->speed_preset];
}

void simulation_session_update(SimulationSession *session, double real_seconds)
{
    if (!session->paused) {
        solar_app_step_system_with_trails(&session->system, &session->trails, &session->clock,
            real_seconds * simulation_session_time_scale(session));
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

BodyInspection simulation_session_inspect(const SimulationSession *session)
{
    const Body *body = &session->system.bodies[session->selected_body_index];
    int parent_index = solar_system_parent_index(&session->system, session->selected_body_index);
    BodyInspection result = {.name = body->name, .parent_name = "None", .has_parent = parent_index >= 0,
        .mass_kg = body->mass_kg, .radius_m = body->radius_m};
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
