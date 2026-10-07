#ifndef SOLAR_SYSTEM_H
#define SOLAR_SYSTEM_H

#include <stddef.h>

#include "body.h"
#include "satellite_catalog.h"

/* Core scene inventory (V5): Sun; Mercury, Venus, Earth, Moon, Mars, Phobos,
 * Deimos, Vesta, Jupiter; Jupiter's catalogued moons; Saturn, Uranus, Neptune. */
#define SOLAR_CORE_NAMED_BODY_COUNT 13
#define SOLAR_CORE_SCENE_BODY_COUNT (SOLAR_CORE_NAMED_BODY_COUNT + SOLAR_JOVIAN_MOON_COUNT)
/* Every scene shares one fixed array; the core scene is the largest. Other
 * scenes (lessons, catalog experiments) assert that they fit at compile time. */
#define SOLAR_SYSTEM_BODY_CAPACITY SOLAR_CORE_SCENE_BODY_COUNT

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
Body solar_system_create_vesta_at_perihelion(void);
Body solar_system_create_jupiter_at_perihelion(void);
Body solar_system_create_saturn_at_perihelion(void);
Body solar_system_create_uranus_at_perihelion(void);
Body solar_system_create_neptune_at_perihelion(void);
SolarSystem solar_system_create_sun_only(void);
SolarSystem solar_system_create_sun_mercury(void);
SolarSystem solar_system_create_sun_vesta(void);
SolarSystem solar_system_create_sun_jupiter(void);
SolarSystem solar_system_create_sun_saturn(void);
SolarSystem solar_system_create_sun_mercury_venus(void);
SolarSystem solar_system_create_sun_mercury_venus_earth(void);
SolarSystem solar_system_create_sun_mercury_venus_earth_moon(void);
SolarSystem solar_system_create_sun_mercury_venus_earth_moon_mars(void);
SolarSystem solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos(void);
SolarSystem solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos_vesta(void);
SolarSystem solar_system_create_sun_mercury_venus_earth_moon_mars_phobos_deimos_vesta_jupiter(void);
/* Adds one body if capacity remains. A full scene returns false unchanged. */
bool solar_system_append(SolarSystem *system, const Body *body);
void solar_system_step(SolarSystem *system, double dt_seconds);
SolarSystem solar_system_create_current(void);
int solar_system_parent_index(const SolarSystem *system, size_t body_index);

#endif
