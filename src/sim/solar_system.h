#ifndef SOLAR_SYSTEM_H
#define SOLAR_SYSTEM_H

#include <stddef.h>

#include "body.h"

#define SOLAR_SYSTEM_BODY_CAPACITY 8

typedef struct SolarSystem {
    Body bodies[SOLAR_SYSTEM_BODY_CAPACITY];
    size_t body_count;
    double elapsed_seconds;
} SolarSystem;

Body solar_system_create_mercury_at_perihelion(void);
Body solar_system_create_venus_at_perihelion(void);
Body solar_system_create_earth_at_perihelion(void);
Body solar_system_create_moon_at_perigee_near_earth(const Body *earth);
Body solar_system_create_mars_at_perihelion(void);
Body solar_system_create_phobos_at_periareion_near_mars(const Body *mars);
Body solar_system_create_deimos_at_periareion_near_mars(const Body *mars);
SolarSystem solar_system_create_sun_only(void);
SolarSystem solar_system_create_sun_mercury(void);
SolarSystem solar_system_create_sun_mercury_venus(void);
SolarSystem solar_system_create_sun_mercury_venus_earth(void);
SolarSystem solar_system_create_sun_mercury_venus_earth_moon(void);
SolarSystem solar_system_create_sun_mercury_venus_earth_moon_mars(void);
SolarSystem solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos(void);
void solar_system_step(SolarSystem *system, double dt_seconds);

#endif
