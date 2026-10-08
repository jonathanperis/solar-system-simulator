#ifndef SOLAR_SYSTEM_H
#define SOLAR_SYSTEM_H

#include <stddef.h>

#include "body.h"
#include "satellite_catalog.h"

/* Main scene inventory (V5, SPEC 2026-10-07 scene split): the large bodies.
 * Sun; Mercury, Venus, Earth, Moon, Mars, Phobos, Deimos, Vesta, Jupiter and
 * its 4 major moons; Saturn and 7; Uranus and 5; Neptune and Triton; the dwarf
 * planet Pluto and Charon. Every other moon lives only in its primary's family
 * scene. */
#define SOLAR_MAJOR_MOON_COUNT 18
#define SOLAR_CORE_SCENE_BODY_COUNT (14 + SOLAR_MAJOR_MOON_COUNT)
/* A family scene: the Sun, the eight planets and one primary's complete moon
 * catalog (major moons first). Pluto, Didymos and Patroclus, which are not
 * planets, sit at index 9 ahead of their moons. Saturn's scene is the largest. */
#define SOLAR_FAMILY_SCENE_PLANET_COUNT 9
#define SOLAR_LARGEST_FAMILY_SCENE_BODY_COUNT (SOLAR_FAMILY_SCENE_PLANET_COUNT + SOLAR_SATURNIAN_MOON_COUNT)
/* Every scene shares one fixed array sized for the largest scene. Other
 * scenes (lessons, catalog experiments) assert that they fit at compile time. */
#define SOLAR_SYSTEM_BODY_CAPACITY SOLAR_LARGEST_FAMILY_SCENE_BODY_COUNT

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
Body solar_system_create_pluto_at_perihelion(void);
Body solar_system_create_didymos_at_perihelion(void);
Body solar_system_create_patroclus_at_perihelion(void);
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
/* The main scene (large bodies only). */
SolarSystem solar_system_create_current(void);
/* Family scene of a giant planet, Pluto, Didymos or Patroclus: Sun at index
 * 0, then Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus and Neptune
 * (1-8), Pluto, Didymos or Patroclus at 9 when that is the primary, then the primary's moons,
 * major moons first, each group in catalog order. False for any other body. */
bool solar_system_create_family(BodyId primary, SolarSystem *result);
/* Index of the primary in its family scene (Jupiter 5 ... Neptune 8,
 * Pluto, Didymos and Patroclus 9), or -1. */
int solar_system_family_planet_index(BodyId primary);
int solar_system_parent_index(const SolarSystem *system, size_t body_index);

#endif
