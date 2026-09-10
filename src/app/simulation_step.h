#ifndef SOLAR_APP_SIMULATION_STEP_H
#define SOLAR_APP_SIMULATION_STEP_H

#include "body_trails.h"
#include "../sim/solar_system.h"

/* Verified against 100-day orbital phase and half-step convergence tests. */
#define SOLAR_APP_MAX_PHYSICS_STEP_SECONDS 15.0

typedef struct SimulationClock {
    double pending_seconds;
} SimulationClock;

void solar_app_step_system_with_trails(
    SolarSystem *system,
    BodyTrails *trails,
    SimulationClock *clock,
    double dt_seconds,
    size_t max_steps
);

#endif
