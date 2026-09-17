#ifndef SOLAR_COLLISIONS_H
#define SOLAR_COLLISIONS_H
#include "solar_system.h"

typedef enum CollisionMode { COLLISION_NONE, COLLISION_BOUNCE, COLLISION_MERGE } CollisionMode;

/* Contact response for the deliberately bounded, head-on two-sphere lesson. */
bool collision_resolve_pair(SolarSystem *system, CollisionMode mode, double *kinetic_loss_j);
const char *collision_mode_name(CollisionMode mode);
#endif
