#ifndef SOLAR_BODY_TRAILS_H
#define SOLAR_BODY_TRAILS_H

#include <stddef.h>

#include "../sim/solar_system.h"

#define SOLAR_TRAIL_POINT_CAPACITY 512

typedef struct BodyTrail {
    Vec3d points[SOLAR_TRAIL_POINT_CAPACITY];
    size_t start;
    size_t count;
} BodyTrail;

typedef struct BodyTrails {
    BodyTrail trails[SOLAR_SYSTEM_BODY_CAPACITY];
} BodyTrails;

BodyTrails body_trails_create(void);
void body_trails_record_system(BodyTrails *trails, const SolarSystem *system);
size_t body_trails_point_count(const BodyTrails *trails, size_t body_index);
Vec3d body_trails_point_at(const BodyTrails *trails, size_t body_index, size_t point_index);

#endif
