#ifndef SOLAR_APP_SIMULATION_STEP_H
#define SOLAR_APP_SIMULATION_STEP_H

#include "body_trails.h"
#include "../sim/solar_system.h"
#include "../sim/physics.h"
#include "../sim/collisions.h"
#include <stdint.h>

/* Verified against 100-day orbital phase and half-step convergence tests. */
#define SOLAR_APP_MAX_PHYSICS_STEP_SECONDS 15.0

typedef struct SimulationClock {
    double pending_seconds;
    double step_seconds; /* Zero-initialized clocks use the core 15-second step. */
    PhysicsIntegrator integrator;
    uint64_t ticks;
    CollisionMode collision_mode;
    uint64_t collision_count;
    double dissipated_energy_j;
} SimulationClock;

double simulation_clock_step_seconds(const SimulationClock *clock);
/* Accelerations must be current; every integrator/contact step leaves them so. */
void simulation_clock_tick(SolarSystem *system, BodyTrails *trails, SimulationClock *clock);

void solar_app_step_system_with_trails(
    SolarSystem *system,
    BodyTrails *trails,
    SimulationClock *clock,
    double dt_seconds,
    size_t max_steps
);

#endif
