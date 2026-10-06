#ifndef SOLAR_APP_SIMULATION_STEP_H
#define SOLAR_APP_SIMULATION_STEP_H

#include "body_trails.h"
#include "../sim/solar_system.h"
#include "../sim/physics.h"
#include "../sim/collisions.h"
#include <stdbool.h>
#include <stdint.h>

/* Verified against 100-day orbital phase and half-step convergence tests. */
#define SOLAR_APP_MAX_PHYSICS_STEP_SECONDS 15.0

/* The interactive loop targets 60 fps, so ordinary frames last ~0.017 s and
 * even badly overloaded ones stay well under a second. A longer single frame
 * means the loop was not running at all: laptop sleep with the window visible,
 * a debugger breakpoint, a blocked window drag. That wall time is discarded like
 * a hidden-tab resume. At 15 days/s, integrating an 8-hour sleep would queue
 * ~3.7e10 s (hours of full-CPU catch-up at the per-update step cap). Frames at
 * or below this threshold are never dropped, so slow hardware still shows up as
 * pending time and a lower achieved speed instead of being hidden. */
#define SOLAR_APP_STALL_FRAME_SECONDS 1.0

typedef struct SimulationClock {
    double pending_seconds;
    double step_seconds; /* Zero-initialized clocks use the core 15-second step. */
    PhysicsIntegrator integrator;
    uint64_t ticks;
    CollisionMode collision_mode;
    uint64_t collision_count;
    double dissipated_energy_j;
    /* Lessons only (A54): point-mass gravity has no surface, so a run that
     * brings two spheres into contact is flagged at its first contact tick
     * (0 = none) and its analytical lesson errors stop being published. */
    bool monitor_contact;
    uint64_t contact_tick;
} SimulationClock;

double simulation_clock_step_seconds(const SimulationClock *clock);
/* True for a frame delta that is not playback time: longer than the stall
 * threshold, negative, or not finite (a NaN would poison the accumulator). */
bool simulation_frame_is_stall(double real_seconds);
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
