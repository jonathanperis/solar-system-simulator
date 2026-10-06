#ifndef SOLAR_COLLISIONS_H
#define SOLAR_COLLISIONS_H
#include "solar_system.h"

typedef enum CollisionMode { COLLISION_NONE, COLLISION_BOUNCE, COLLISION_MERGE } CollisionMode;

/* Lessons have at most three bodies; larger scenes are not contact-monitored. */
#define SOLAR_CONTACT_MONITOR_MAX_BODIES 3

/* True if, moving in straight lines from `before_positions` to their current
 * positions during one step, any two spheres came closer than the sum of
 * their radii (unknown radius counts as a point). The swept check catches a
 * fast pass that would skip over a sphere between two sampled ticks. */
bool collision_contact_during_step(const Vec3d *before_positions, const SolarSystem *system);

/* Contact response for the deliberately bounded, head-on two-sphere lesson. */
bool collision_resolve_pair(SolarSystem *system, CollisionMode mode, double *kinetic_loss_j);
const char *collision_mode_name(CollisionMode mode);
#endif
