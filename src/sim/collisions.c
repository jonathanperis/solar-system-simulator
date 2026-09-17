#include "collisions.h"
#include <math.h>

const char *collision_mode_name(CollisionMode mode)
{
    return mode == COLLISION_BOUNCE ? "bounce" : mode == COLLISION_MERGE ? "merge" : "none";
}

bool collision_resolve_pair(SolarSystem *system, CollisionMode mode, double *kinetic_loss_j)
{
    *kinetic_loss_j = 0;
    if (mode == COLLISION_NONE || system->body_count != 2) return false;
    Body *a = &system->bodies[0], *b = &system->bodies[1];
    Vec3d displacement = vec3d_sub(b->position_m, a->position_m);
    double distance = vec3d_length(displacement), contact = a->radius_m + b->radius_m;
    if (a->fixed || b->fixed || a->mass_kg <= 0 || b->mass_kg <= 0 || distance <= 0 || distance > contact) return false;
    Vec3d normal = vec3d_scale(displacement, 1 / distance);
    double closing_speed = vec3d_dot(vec3d_sub(b->velocity_mps, a->velocity_mps), normal);
    if (closing_speed >= 0) return false;
    double total_mass = a->mass_kg + b->mass_kg;
    if (mode == COLLISION_MERGE) {
        double before = .5 * a->mass_kg * vec3d_length_squared(a->velocity_mps)
            + .5 * b->mass_kg * vec3d_length_squared(b->velocity_mps);
        a->position_m = vec3d_scale(vec3d_add(vec3d_scale(a->position_m, a->mass_kg), vec3d_scale(b->position_m, b->mass_kg)), 1 / total_mass);
        a->velocity_mps = vec3d_scale(vec3d_add(vec3d_scale(a->velocity_mps, a->mass_kg), vec3d_scale(b->velocity_mps, b->mass_kg)), 1 / total_mass);
        a->radius_m = cbrt(pow(a->radius_m, 3) + pow(b->radius_m, 3));
        a->mass_kg = total_mass;
        a->name = "Merged colliders";
        system->body_count = 1;
        *kinetic_loss_j = before - .5 * total_mass * vec3d_length_squared(a->velocity_mps);
    } else {
        /* Frictionless elastic impulse: equal/opposite momentum changes. The
         * small separation correction keeps the pair's center of mass fixed. */
        double impulse = -2 * closing_speed / (1 / a->mass_kg + 1 / b->mass_kg);
        a->velocity_mps = vec3d_sub(a->velocity_mps, vec3d_scale(normal, impulse / a->mass_kg));
        b->velocity_mps = vec3d_add(b->velocity_mps, vec3d_scale(normal, impulse / b->mass_kg));
        a->position_m = vec3d_sub(a->position_m, vec3d_scale(normal, (contact - distance) * b->mass_kg / total_mass));
        b->position_m = vec3d_add(b->position_m, vec3d_scale(normal, (contact - distance) * a->mass_kg / total_mass));
    }
    return true;
}
