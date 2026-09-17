#include "physics.h"

#include <math.h>

#include "constants.h"

Vec3d gravitational_acceleration_from(const Body *target, const Body *source)
{
    if (target == source) {
        return vec3d_zero();
    }

    Vec3d displacement = vec3d_sub(source->position_m, target->position_m);
    double distance_squared = vec3d_length_squared(displacement);
    if (distance_squared == 0.0) {
        return vec3d_zero();
    }

    double distance = sqrt(distance_squared);
    /* Newtonian point-mass gravity: a = G*M*r_hat/r^2. Multiplying the
     * displacement vector by 1/r^3 preserves the direction toward the source
     * while producing acceleration in SI units (m/s^2). */
    double scale = SOLAR_G * source->mass_kg / (distance_squared * distance);
    return vec3d_scale(displacement, scale);
}

void physics_compute_accelerations(Body *bodies, size_t body_count)
{
    for (size_t i = 0; i < body_count; ++i) {
        bodies[i].acceleration_mps2 = vec3d_zero();
    }

    /* Most new satellites are explicitly massless tracers. Iterate sources
     * first to skip their entire zero-contribution columns, while preserving
     * the original source summation order for every target. Direct component
     * arithmetic keeps this measured hot loop free of cross-file vector calls. */
    for (size_t j = 0; j < body_count; ++j) {
        if (bodies[j].mass_kg == 0.0) continue;
        const Vec3d source = bodies[j].position_m;
        const double gm = SOLAR_G * bodies[j].mass_kg;
        for (size_t i = 0; i < body_count; ++i) {
            if (i == j) continue;
            double x = source.x - bodies[i].position_m.x;
            double y = source.y - bodies[i].position_m.y;
            double z = source.z - bodies[i].position_m.z;
            double r2 = x * x + y * y + z * z;
            if (r2 == 0.0) continue;
            double scale = gm / (r2 * sqrt(r2));
            bodies[i].acceleration_mps2.x += x * scale;
            bodies[i].acceleration_mps2.y += y * scale;
            bodies[i].acceleration_mps2.z += z * scale;
        }
    }
}

void physics_step(Body *bodies, size_t body_count, double dt_seconds)
{
    physics_compute_accelerations(bodies, body_count);
    physics_step_from_accelerations(bodies, body_count, dt_seconds);
}

void physics_step_with_integrator(Body *bodies, size_t body_count, double dt_seconds, PhysicsIntegrator method)
{
    physics_compute_accelerations(bodies, body_count);
    if (method == PHYSICS_EULER) physics_step_euler_from_accelerations(bodies, body_count, dt_seconds);
    else physics_step_from_accelerations(bodies, body_count, dt_seconds);
}

void physics_step_euler_from_accelerations(Body *bodies, size_t body_count, double dt_seconds)
{
    /* Explicit Euler is an intentionally less accurate classroom comparison:
     * drift with the old velocity, then kick with the old acceleration. */
    for (size_t i = 0; i < body_count; ++i) {
        if (bodies[i].fixed) continue;
        bodies[i].position_m = vec3d_add(bodies[i].position_m, vec3d_scale(bodies[i].velocity_mps, dt_seconds));
        bodies[i].velocity_mps = vec3d_add(bodies[i].velocity_mps, vec3d_scale(bodies[i].acceleration_mps2, dt_seconds));
    }
    physics_compute_accelerations(bodies, body_count);
}

void physics_step_from_accelerations(Body *bodies, size_t body_count, double dt_seconds)
{
    /* Velocity-Verlet: half-kick velocities, drift positions, recompute
     * acceleration from the new positions, then finish the second half-kick.
     * This is more orbit-friendly than explicit Euler for the same simple API. */
    for (size_t i = 0; i < body_count; ++i) {
        if (bodies[i].fixed) {
            continue;
        }

        Vec3d half_kick = vec3d_scale(bodies[i].acceleration_mps2, 0.5 * dt_seconds);
        bodies[i].velocity_mps = vec3d_add(bodies[i].velocity_mps, half_kick);
        bodies[i].position_m = vec3d_add(bodies[i].position_m, vec3d_scale(bodies[i].velocity_mps, dt_seconds));
    }

    physics_compute_accelerations(bodies, body_count);

    for (size_t i = 0; i < body_count; ++i) {
        if (bodies[i].fixed) {
            continue;
        }

        Vec3d half_kick = vec3d_scale(bodies[i].acceleration_mps2, 0.5 * dt_seconds);
        bodies[i].velocity_mps = vec3d_add(bodies[i].velocity_mps, half_kick);
    }
}
