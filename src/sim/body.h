#ifndef SOLAR_BODY_H
#define SOLAR_BODY_H

#include <stdbool.h>

#include "vec3d.h"

typedef enum BodyKind {
    BODY_KIND_STAR,
    BODY_KIND_PLANET,
    BODY_KIND_MOON,
    BODY_KIND_ASTEROID,
    BODY_KIND_DWARF_PLANET
} BodyKind;

typedef enum BodyId {
    BODY_ID_UNKNOWN = -2,
    BODY_ID_NONE = -1,
    /* Legacy demonstration IDs are their original scene indices. */
    BODY_ID_SUN = 0,
    BODY_ID_MERCURY = 1,
    BODY_ID_VENUS = 2,
    BODY_ID_EARTH = 3,
    BODY_ID_MOON = 4,
    BODY_ID_MARS = 5,
    BODY_ID_PHOBOS = 6,
    BODY_ID_DEIMOS = 7,
    BODY_ID_VESTA = 8,
    BODY_ID_JUPITER = 9,
    /* New satellites retain JPL codes, not their position in a sorted menu. */
    BODY_ID_IO = 501,
    BODY_ID_EUROPA = 502,
    BODY_ID_GANYMEDE = 503,
    BODY_ID_CALLISTO = 504,
    BODY_ID_ENCELADUS = 602,
    BODY_ID_TITAN = 606,
    BODY_ID_TRITON = 801,
    BODY_ID_CHARON = 901,
    /* NAIF's planet-center code cannot collide with 6xx Saturnian moons. */
    BODY_ID_SATURN = 699,
    BODY_ID_URANUS = 799,
    BODY_ID_NEPTUNE = 899,
    BODY_ID_PLUTO = 999,
    /* Horizons IDs of the Didymos binary (primary centre and Dimorphos). */
    BODY_ID_DIMORPHOS = 120065803,
    BODY_ID_DIDYMOS = 920065803
} BodyId;

typedef enum PhysicalQuality {
    PHYSICAL_MEASURED,
    PHYSICAL_ESTIMATED,
    PHYSICAL_UNKNOWN,
    PHYSICAL_PUBLISHED
} PhysicalQuality;

/*
 * Body is pure simulation state: all distances are meters, velocities are
 * meters/second, and acceleration is meters/second^2. The id/parent_id fields
 * are stable catalog metadata for app/render/docs layers; physics still works
 * from mass and position instead of hard-coded parent relationships.
 */
typedef struct Body {
    /* Borrowed strings: literals/catalog storage outlive the body. Imported
     * names belong to SimulationSession; copying a Body does not copy text. */
    const char *name;
    BodyKind kind;
    BodyId id;
    BodyId parent_id;
    double mass_kg;
    double radius_m;
    PhysicalQuality mass_quality;
    PhysicalQuality radius_quality;
    const char *group;
    Vec3d position_m;
    Vec3d velocity_mps;
    Vec3d acceleration_mps2;
    bool fixed;
    /* Oblateness (SPEC A94): zonal harmonic J2 referenced to
     * `j2_radius_m`, about the unit spin axis `pole` (simulation axes). Zero
     * means a point mass. Set only on planets in astronomy scenes; guided
     * lessons stay point masses so analytic Kepler references still apply. */
    double j2;
    double j2_radius_m;
    Vec3d pole;
} Body;

Body body_create(
    const char *name,
    BodyKind kind,
    double mass_kg,
    double radius_m,
    Vec3d position_m,
    Vec3d velocity_mps,
    bool fixed
);

Body body_create_identified(
    const char *name,
    BodyKind kind,
    BodyId id,
    BodyId parent_id,
    double mass_kg,
    double radius_m,
    Vec3d position_m,
    Vec3d velocity_mps,
    bool fixed
);

#endif
