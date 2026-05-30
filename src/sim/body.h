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

typedef struct Body {
    const char *name;
    BodyKind kind;
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

#endif
