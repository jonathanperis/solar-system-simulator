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
    double scale = SOLAR_G * source->mass_kg / (distance_squared * distance);
    return vec3d_scale(displacement, scale);
}

void physics_compute_accelerations(Body *bodies, size_t body_count)
{
    for (size_t i = 0; i < body_count; ++i) {
        bodies[i].acceleration_mps2 = vec3d_zero();
    }

    for (size_t i = 0; i < body_count; ++i) {
        for (size_t j = 0; j < body_count; ++j) {
            Vec3d contribution = gravitational_acceleration_from(&bodies[i], &bodies[j]);
            bodies[i].acceleration_mps2 = vec3d_add(bodies[i].acceleration_mps2, contribution);
        }
    }
}

void physics_step(Body *bodies, size_t body_count, double dt_seconds)
{
    physics_compute_accelerations(bodies, body_count);

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
