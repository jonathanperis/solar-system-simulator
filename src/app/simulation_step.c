#include "simulation_step.h"
#include "../sim/physics.h"

void solar_app_step_system_with_trails(
    SolarSystem *system,
    BodyTrails *trails,
    SimulationClock *clock,
    double dt_seconds,
    size_t max_steps
)
{
    if (dt_seconds < 0.0) {
        return;
    }

    /* Never integrate a short remainder just because a render frame ended.
     * Keep it for the next frame so trajectories do not depend on frame rate. */
    clock->pending_seconds += dt_seconds;
    /* The final acceleration of one Verlet step is the initial acceleration
     * of the next. Prime once per batch, never cache across external edits. */
    if (max_steps && clock->pending_seconds >= SOLAR_APP_MAX_PHYSICS_STEP_SECONDS) {
        physics_compute_accelerations(system->bodies, system->body_count);
    }
    /* A bounded batch returns control to the renderer/input loop even when
     * hardware cannot keep up. Zero new time may drain existing pending work. */
    for (size_t steps = 0; steps < max_steps && clock->pending_seconds >= SOLAR_APP_MAX_PHYSICS_STEP_SECONDS; ++steps) {
        physics_step_from_accelerations(system->bodies, system->body_count, SOLAR_APP_MAX_PHYSICS_STEP_SECONDS);
        system->elapsed_seconds += SOLAR_APP_MAX_PHYSICS_STEP_SECONDS;
        body_trails_record_system(trails, system);
        clock->pending_seconds -= SOLAR_APP_MAX_PHYSICS_STEP_SECONDS;
    }
}
