#include "simulation_step.h"

void solar_app_step_system_with_trails(
    SolarSystem *system,
    BodyTrails *trails,
    double dt_seconds,
    double max_step_seconds
)
{
    if (dt_seconds <= 0.0 || max_step_seconds <= 0.0) {
        return;
    }

    while (dt_seconds > max_step_seconds) {
        solar_system_step(system, max_step_seconds);
        body_trails_record_system(trails, system);
        dt_seconds -= max_step_seconds;
    }

    if (dt_seconds > 0.0) {
        solar_system_step(system, dt_seconds);
        body_trails_record_system(trails, system);
    }
}
