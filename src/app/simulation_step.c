#include "simulation_step.h"
#include "../sim/physics.h"

double simulation_clock_step_seconds(const SimulationClock *clock)
{
    return clock->step_seconds > 0 ? clock->step_seconds : SOLAR_APP_MAX_PHYSICS_STEP_SECONDS;
}

void simulation_clock_tick(SolarSystem *system, BodyTrails *trails, SimulationClock *clock)
{
    double step = simulation_clock_step_seconds(clock);
    if (clock->integrator == PHYSICS_EULER) physics_step_euler_from_accelerations(system->bodies, system->body_count, step);
    else physics_step_from_accelerations(system->bodies, system->body_count, step);
    size_t count = system->body_count;
    double loss;
    if (collision_resolve_pair(system, clock->collision_mode, &loss)) {
        ++clock->collision_count;
        clock->dissipated_energy_j += loss;
        physics_compute_accelerations(system->bodies, system->body_count);
        if (trails && count != system->body_count) {
            double interval = trails->sample_interval_seconds;
            body_trails_destroy(trails);
            trails->sample_interval_seconds = interval;
        }
    }
    ++clock->ticks;
    system->elapsed_seconds = (double)clock->ticks * step;
    if (trails) body_trails_record_system(trails, system);
}

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
    double step = simulation_clock_step_seconds(clock);
    if (max_steps && clock->pending_seconds >= step) {
        physics_compute_accelerations(system->bodies, system->body_count);
    }
    /* A bounded batch returns control to the renderer/input loop even when
     * hardware cannot keep up. Zero new time may drain existing pending work. */
    for (size_t steps = 0; steps < max_steps && clock->pending_seconds >= step; ++steps) {
        simulation_clock_tick(system, trails, clock);
        clock->pending_seconds -= step;
    }
}
