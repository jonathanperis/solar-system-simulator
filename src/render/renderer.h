#ifndef SOLAR_RENDERER_H
#define SOLAR_RENDERER_H

#include <stddef.h>

#include <raylib.h>

#include "../app/body_trails.h"
#include "../sim/solar_system.h"

#define SOLAR_RENDER_MAX_TRAIL_SEGMENTS 1024

typedef enum RenderScaleMode {
    RENDER_SCALE_ILLUSTRATIVE,
    RENDER_SCALE_REAL
} RenderScaleMode;

typedef struct RenderSystemFrame {
    size_t root_index;
    double radius;
} RenderSystemFrame;

RenderSystemFrame renderer_system_frame(const SolarSystem *system, size_t selected, RenderScaleMode mode);
RenderSystemFrame renderer_body_frame(const SolarSystem *system, size_t selected, RenderScaleMode mode);

const char *renderer_scale_mode_label(RenderScaleMode mode);
Color renderer_body_color(const Body *body);
Vec3d renderer_body_position(const SolarSystem *system, size_t body_index, RenderScaleMode mode);
Vec3d renderer_trail_point_position(const SolarSystem *system, const BodyTrails *trails, size_t body_index, size_t point_index, RenderScaleMode mode);
size_t renderer_trail_sample_stride(size_t point_count);
size_t renderer_trail_draw_segment_count(size_t point_count);
float renderer_body_radius(const Body *body, RenderScaleMode mode);
float renderer_body_visual_radius(const Body *body, RenderScaleMode mode);
int renderer_grid_slices_for_system(const SolarSystem *system, RenderScaleMode mode);
void renderer_draw_solar_system(const SolarSystem *system, const BodyTrails *trails, RenderScaleMode mode);

#endif
