#ifndef SOLAR_SYSTEM_H
#define SOLAR_SYSTEM_H

#include <stddef.h>

#include "body.h"

#define SOLAR_SYSTEM_BODY_CAPACITY 3

typedef struct SolarSystem {
    Body bodies[SOLAR_SYSTEM_BODY_CAPACITY];
    size_t body_count;
    double elapsed_seconds;
} SolarSystem;

Body solar_system_create_mercury_at_perihelion(void);
Body solar_system_create_venus_at_perihelion(void);
SolarSystem solar_system_create_sun_only(void);
SolarSystem solar_system_create_sun_mercury(void);
SolarSystem solar_system_create_sun_mercury_venus(void);
void solar_system_step(SolarSystem *system, double dt_seconds);

#endif
