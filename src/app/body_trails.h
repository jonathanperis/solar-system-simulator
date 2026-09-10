#ifndef SOLAR_BODY_TRAILS_H
#define SOLAR_BODY_TRAILS_H

#include <stddef.h>
#include <stdbool.h>

#include "../sim/solar_system.h"

#define SOLAR_TRAIL_INITIAL_CAPACITY 512
#define SOLAR_TRAIL_MAX_POINTS 1025
#define SOLAR_TRAIL_INITIAL_INTERVAL_SECONDS 300.0

typedef struct BodyTrail {
    Vec3d *points;
    size_t count;
    size_t capacity;
    Vec3d latest_position_m;
} BodyTrail;

typedef struct BodyTrails {
    BodyTrail trails[SOLAR_SYSTEM_BODY_CAPACITY];
    bool recording_failed;
    double sample_interval_seconds;
    double next_sample_seconds;
    double last_sample_seconds;
    double latest_seconds;
} BodyTrails;

BodyTrails body_trails_create(void);
void body_trails_destroy(BodyTrails *trails);
void body_trails_record_system(BodyTrails *trails, const SolarSystem *system);
size_t body_trails_point_count(const BodyTrails *trails, size_t body_index);
Vec3d body_trails_point_at(const BodyTrails *trails, size_t body_index, size_t point_index);
bool body_trails_recording_failed(const BodyTrails *trails);

#endif
