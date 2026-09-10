#include "simulation_step.h"

void solar_app_step_system_with_trails(
    SolarSystem *system,
    BodyTrails *trails,
    SimulationClock *clock,
    double dt_seconds
)
{
    if (dt_seconds <= 0.0) {
        return;
    }

    /* Never integrate a short remainder just because a render frame ended.
     * Keep it for the next frame so trajectories do not depend on frame rate. */
    clock->pending_seconds += dt_seconds;
    while (clock->pending_seconds >= SOLAR_APP_MAX_PHYSICS_STEP_SECONDS) {
        solar_system_step(system, SOLAR_APP_MAX_PHYSICS_STEP_SECONDS);
        body_trails_record_system(trails, system);
        clock->pending_seconds -= SOLAR_APP_MAX_PHYSICS_STEP_SECONDS;
    }
}
