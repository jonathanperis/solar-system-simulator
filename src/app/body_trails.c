#include "body_trails.h"

#include <stdlib.h>

static void body_trail_grow(BodyTrail *trail)
{
    size_t new_capacity = trail->capacity == 0 ? SOLAR_TRAIL_INITIAL_CAPACITY : trail->capacity * 2;
    Vec3d *new_points = realloc(trail->points, new_capacity * sizeof(*new_points));
    if (new_points == NULL) {
        abort();
    }

    trail->points = new_points;
    trail->capacity = new_capacity;
}

static void body_trail_append(BodyTrail *trail, Vec3d point)
{
    if (trail->count == trail->capacity) {
        body_trail_grow(trail);
    }

    trail->points[trail->count] = point;
    ++trail->count;
}

BodyTrails body_trails_create(void)
{
    BodyTrails trails = {0};
    return trails;
}

void body_trails_destroy(BodyTrails *trails)
{
    for (size_t i = 0; i < SOLAR_SYSTEM_BODY_CAPACITY; ++i) {
        free(trails->trails[i].points);
        trails->trails[i] = (BodyTrail){0};
    }
}

void body_trails_record_system(BodyTrails *trails, const SolarSystem *system)
{
    /* Trails live in app state, not the physics body, so the simulator can keep
     * all historical positions without constraining integrator state or wrapping
     * a fixed-size buffer during long runs. */
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

    return trail->points[point_index];
}
