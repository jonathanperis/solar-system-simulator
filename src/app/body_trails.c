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

    /* The odd point cap preserves both endpoints. Future sampling must also
     * slow down by two, otherwise repeated compaction erases early curvature. */
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

    return body_trail_grow(trail);
}

static void body_trail_append(BodyTrail *trail, Vec3d point)
{
    trail->points[trail->count] = point;
    ++trail->count;
}

BodyTrails body_trails_create(void)
{
    BodyTrails trails = {.sample_interval_seconds = SOLAR_TRAIL_INITIAL_INTERVAL_SECONDS};
    return trails;
}

void body_trails_destroy(BodyTrails *trails)
{
    for (size_t i = 0; i < SOLAR_SYSTEM_BODY_CAPACITY; ++i) {
        free(trails->trails[i].points);
        trails->trails[i] = (BodyTrail){0};
    }
    *trails = body_trails_create();
}

void body_trails_record_system(BodyTrails *trails, const SolarSystem *system)
{
    if (trails->recording_failed) {
        return;
    }

    /* The moving endpoint is separate from uniformly sampled history. This
     * avoids tying trail memory/resolution to the integrator or display rate. */
    trails->latest_seconds = system->elapsed_seconds;
    for (size_t i = 0; i < system->body_count && i < SOLAR_SYSTEM_BODY_CAPACITY; ++i) {
        trails->trails[i].latest_position_m = system->bodies[i].position_m;
    }
    if (system->elapsed_seconds < trails->next_sample_seconds) {
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

    bool compacted = false;
    for (size_t i = 0; i < system->body_count && i < SOLAR_SYSTEM_BODY_CAPACITY; ++i) {
        if (system->bodies[i].kind == BODY_KIND_STAR) {
            continue;
        }

        body_trail_append(&trails->trails[i], system->bodies[i].position_m);
        if (trails->trails[i].count == SOLAR_TRAIL_MAX_POINTS) {
            body_trail_compact(&trails->trails[i]);
            compacted = true;
        }
    }
    if (compacted) {
        trails->sample_interval_seconds *= 2.0;
    }
    trails->last_sample_seconds = system->elapsed_seconds;
    trails->next_sample_seconds = system->elapsed_seconds + trails->sample_interval_seconds;
}

size_t body_trails_point_count(const BodyTrails *trails, size_t body_index)
{
    if (body_index >= SOLAR_SYSTEM_BODY_CAPACITY) {
        return 0;
    }

    size_t count = trails->trails[body_index].count;
    return count + (count > 0 && trails->latest_seconds > trails->last_sample_seconds ? 1 : 0);
}

Vec3d body_trails_point_at(const BodyTrails *trails, size_t body_index, size_t point_index)
{
    if (body_index >= SOLAR_SYSTEM_BODY_CAPACITY) {
        return vec3d_zero();
    }

    const BodyTrail *trail = &trails->trails[body_index];
    if (point_index >= body_trails_point_count(trails, body_index)) {
        return vec3d_zero();
    }
    if (point_index == trail->count) {
        return trail->latest_position_m;
    }

    return trail->points[point_index];
}

bool body_trails_recording_failed(const BodyTrails *trails)
{
    return trails->recording_failed;
}
