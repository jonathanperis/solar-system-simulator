#ifndef SOLAR_PHYSICS_H
#define SOLAR_PHYSICS_H

#include <stddef.h>

#include "body.h"
#include "vec3d.h"

typedef enum PhysicsIntegrator { PHYSICS_VERLET, PHYSICS_EULER } PhysicsIntegrator;

void physics_step_with_integrator(Body *bodies, size_t body_count, double dt_seconds, PhysicsIntegrator method);
void physics_step_euler_from_accelerations(Body *bodies, size_t body_count, double dt_seconds);

Vec3d gravitational_acceleration_from(const Body *target, const Body *source);
void physics_compute_accelerations(Body *bodies, size_t body_count);
void physics_step(Body *bodies, size_t body_count, double dt_seconds);
/* Requires acceleration at the current positions; leaves acceleration valid
 * at the new positions. Use only within an uninterrupted integration batch. */
void physics_step_from_accelerations(Body *bodies, size_t body_count, double dt_seconds);

#endif
