#ifndef SOLAR_SYSTEM_H
#define SOLAR_SYSTEM_H

#include <stddef.h>

#include "body.h"

typedef struct SolarSystem {
    Body bodies[1];
    size_t body_count;
    double elapsed_seconds;
} SolarSystem;

SolarSystem solar_system_create_sun_only(void);
void solar_system_step(SolarSystem *system, double dt_seconds);

#endif
