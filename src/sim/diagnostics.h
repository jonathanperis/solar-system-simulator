#ifndef SOLAR_DIAGNOSTICS_H
#define SOLAR_DIAGNOSTICS_H

#include "solar_system.h"

typedef struct PhysicsDiagnostics {
    double kinetic_energy_j;
    double potential_energy_j;
    double total_energy_j;
    double total_mass_kg;
    Vec3d momentum_kg_mps;
    Vec3d angular_momentum_kg_m2ps;
    Vec3d center_of_mass_m;
    bool isolated;
} PhysicsDiagnostics;

PhysicsDiagnostics physics_diagnostics(const SolarSystem *system);
typedef struct ForceContribution {
    size_t source_index;
    Vec3d acceleration_mps2;
    double magnitude_mps2;
    double magnitude_fraction;
} ForceContribution;
size_t physics_force_breakdown(const SolarSystem *system, size_t target, ForceContribution *out, size_t capacity);

#endif
