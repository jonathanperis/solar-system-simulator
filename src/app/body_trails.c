#include "body_trails.h"

static void body_trail_append(BodyTrail *trail, Vec3d point)
{
    if (trail->count < SOLAR_TRAIL_POINT_CAPACITY) {
        size_t write_index = (trail->start + trail->count) % SOLAR_TRAIL_POINT_CAPACITY;
        trail->points[write_index] = point;
        ++trail->count;
        return;
    }

    trail->points[trail->start] = point;
    trail->start = (trail->start + 1) % SOLAR_TRAIL_POINT_CAPACITY;
}

BodyTrails body_trails_create(void)
{
    BodyTrails trails = {0};
    return trails;
}

void body_trails_record_system(BodyTrails *trails, const SolarSystem *system)
{
    for (size_t i = 0; i < system->body_count && i < SOLAR_SYSTEM_BODY_CAPACITY; ++i) {
        if (system->bodies[i].kind == BODY_KIND_STAR) {
            continue;
        }

        body_trail_append(&trails->trails[i], system->bodies[i].position_m);
    }
}

size_t body_trails_point_count(const BodyTrails *trails, size_t body_index)
{
    if (body_index >= SOLAR_SYSTEM_BODY_CAPACITY) {
        return 0;
    }

    return trails->trails[body_index].count;
}

Vec3d body_trails_point_at(const BodyTrails *trails, size_t body_index, size_t point_index)
{
    if (body_index >= SOLAR_SYSTEM_BODY_CAPACITY) {
        return vec3d_zero();
    }

    const BodyTrail *trail = &trails->trails[body_index];
    if (point_index >= trail->count) {
        return vec3d_zero();
    }

    size_t stored_index = (trail->start + point_index) % SOLAR_TRAIL_POINT_CAPACITY;
    return trail->points[stored_index];
}
