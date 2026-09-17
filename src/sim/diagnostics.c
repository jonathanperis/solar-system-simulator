#include "diagnostics.h"
#include "constants.h"
#include "physics.h"
#include <stdlib.h>

static int descending_force(const void *left, const void *right)
{
    const ForceContribution *a = left, *b = right;
    if (a->magnitude_mps2 != b->magnitude_mps2) return a->magnitude_mps2 < b->magnitude_mps2 ? 1 : -1;
    return a->source_index > b->source_index ? 1 : a->source_index < b->source_index ? -1 : 0;
}

size_t physics_force_breakdown(const SolarSystem *system, size_t target, ForceContribution *out, size_t capacity)
{
    if (target >= system->body_count) return 0;
    ForceContribution all[SOLAR_SYSTEM_BODY_CAPACITY];
    size_t count = 0; double sum = 0;
    for (size_t i = 0; i < system->body_count; ++i) {
        if (i == target || system->bodies[i].mass_kg == 0) continue;
        Vec3d acceleration = gravitational_acceleration_from(&system->bodies[target], &system->bodies[i]);
        double magnitude = vec3d_length(acceleration);
        all[count++] = (ForceContribution){i, acceleration, magnitude, 0};
        sum += magnitude;
    }
    qsort(all, count, sizeof(*all), descending_force);
    size_t written = count < capacity ? count : capacity;
    for (size_t i = 0; i < written; ++i) {
        out[i] = all[i];
        out[i].magnitude_fraction = sum > 0 ? all[i].magnitude_mps2 / sum : 0;
    }
    return written;
}

PhysicsDiagnostics physics_diagnostics(const SolarSystem *system)
{
    PhysicsDiagnostics result = {.isolated = true};
    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        if (body->fixed) result.isolated = false;
        if (body->mass_kg == 0) continue;
        Vec3d momentum = vec3d_scale(body->velocity_mps, body->mass_kg);
        result.total_mass_kg += body->mass_kg;
        result.kinetic_energy_j += 0.5 * body->mass_kg * vec3d_length_squared(body->velocity_mps);
        result.momentum_kg_mps = vec3d_add(result.momentum_kg_mps, momentum);
        result.angular_momentum_kg_m2ps = vec3d_add(result.angular_momentum_kg_m2ps,
            vec3d_cross(body->position_m, momentum));
        result.center_of_mass_m = vec3d_add(result.center_of_mass_m, vec3d_scale(body->position_m, body->mass_kg));
        /* Each massive pair contributes once. Tracers have no backreaction and
         * therefore no contribution to the system's energy or momentum. */
        for (size_t j = 0; j < i; ++j) {
            double distance = vec3d_length(vec3d_sub(body->position_m, system->bodies[j].position_m));
            if (distance > 0) result.potential_energy_j -= SOLAR_G * body->mass_kg * system->bodies[j].mass_kg / distance;
        }
    }
    if (result.total_mass_kg > 0) result.center_of_mass_m = vec3d_scale(result.center_of_mass_m, 1.0 / result.total_mass_kg);
    result.total_energy_j = result.kinetic_energy_j + result.potential_energy_j;
    return result;
}
