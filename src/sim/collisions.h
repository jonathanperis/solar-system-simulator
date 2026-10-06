#ifndef SOLAR_COLLISIONS_H
#define SOLAR_COLLISIONS_H
#include "solar_system.h"

typedef enum CollisionMode { COLLISION_NONE, COLLISION_BOUNCE, COLLISION_MERGE } CollisionMode;

/* True if, moving in straight lines from `before_positions` (one per body) to
 * their current positions during one step, any two spheres came closer than
 * the sum of their radii (unknown radius counts as a point). The swept check
 * catches a fast pass that skips over a sphere between two sampled ticks; it
 * can also fire when a coarse step's chord cuts across a curved arc that
 * never entered the sphere, so callers treat it as conservative. */
bool collision_contact_during_step(const Vec3d *before_positions, const SolarSystem *system);

/* Contact response for the deliberately bounded, head-on two-sphere lesson. */
bool collision_resolve_pair(SolarSystem *system, CollisionMode mode, double *kinetic_loss_j);
const char *collision_mode_name(CollisionMode mode);
#endif
