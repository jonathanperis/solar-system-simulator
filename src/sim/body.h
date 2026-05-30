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
    BODY_ID_SUN,
    BODY_ID_MERCURY,
    BODY_ID_VENUS,
    BODY_ID_EARTH,
    BODY_ID_MOON,
    BODY_ID_MARS,
    BODY_ID_PHOBOS,
    BODY_ID_DEIMOS
} BodyId;

/*
 * Body is pure simulation state: all distances are meters, velocities are
 * meters/second, and acceleration is meters/second^2. The id/parent_id fields
 * are stable catalog metadata for app/render/docs layers; physics still works
 * from mass and position instead of hard-coded parent relationships.
 */
typedef struct Body {
    const char *name;
    BodyKind kind;
    BodyId id;
    BodyId parent_id;
    double mass_kg;
    double radius_m;
    Vec3d position_m;
    Vec3d velocity_mps;
    Vec3d acceleration_mps2;
    bool fixed;
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
