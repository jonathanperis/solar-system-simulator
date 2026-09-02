#include "body_trails.h"

#include <stdlib.h>

static bool body_trail_grow(BodyTrail *trail)
{
    size_t new_capacity = trail->capacity == 0 ? SOLAR_TRAIL_INITIAL_CAPACITY : trail->capacity * 2;
    if (new_capacity > SOLAR_TRAIL_MAX_POINTS) {
        new_capacity = SOLAR_TRAIL_MAX_POINTS;
    }

    Vec3d *new_points = realloc(trail->points, new_capacity * sizeof(*new_points));
    if (new_points == NULL) {
        return false;
    }

    trail->points = new_points;
    trail->capacity = new_capacity;
    return true;
}

static void body_trail_compact(BodyTrail *trail)
{
    size_t write_index = 0;

    /* Preserve first sample and every other historical sample. New samples then
     * refill the freed half at full resolution, retaining whole-run shape with
     * bounded memory instead of discarding the beginning of an orbit. */
    for (size_t read_index = 0; read_index < trail->count; read_index += 2) {
        trail->points[write_index] = trail->points[read_index];
        ++write_index;
    }

    trail->count = write_index;
}

static bool body_trail_prepare_append(BodyTrail *trail)
{
    if (trail->count < trail->capacity) {
        return true;
    }

    if (trail->capacity < SOLAR_TRAIL_MAX_POINTS) {
        return body_trail_grow(trail);
    }

    body_trail_compact(trail);
    return true;
}

static void body_trail_append(BodyTrail *trail, Vec3d point)
{
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
    trails->recording_failed = false;
}

void body_trails_record_system(BodyTrails *trails, const SolarSystem *system)
{
    /* Trails live in app state, not the physics body, so the simulator can keep
     * all historical positions without constraining integrator state or wrapping
     * a fixed-size buffer during long runs. */
    if (trails->recording_failed) {
        return;
    }

    /* Reserve every non-star trail before recording any point. A failed resize
     * therefore leaves all body histories on the same sample index. */
    for (size_t i = 0; i < system->body_count && i < SOLAR_SYSTEM_BODY_CAPACITY; ++i) {
        if (system->bodies[i].kind == BODY_KIND_STAR) {
            continue;
        }

        if (!body_trail_prepare_append(&trails->trails[i])) {
            trails->recording_failed = true;
            return;
        }
    }

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

bool body_trails_recording_failed(const BodyTrails *trails)
{
    return trails->recording_failed;
}
