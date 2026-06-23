#ifndef SOLAR_APP_SIMULATION_STEP_H
#define SOLAR_APP_SIMULATION_STEP_H

#include "body_trails.h"
#include "../sim/solar_system.h"

#define SOLAR_APP_MAX_PHYSICS_STEP_SECONDS (5.0 * 60.0)

void solar_app_step_system_with_trails(
    SolarSystem *system,
    BodyTrails *trails,
    double dt_seconds,
    double max_step_seconds
);

#endif
