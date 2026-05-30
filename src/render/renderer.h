#ifndef SOLAR_RENDERER_H
#define SOLAR_RENDERER_H

#include "../app/body_trails.h"
#include "../sim/solar_system.h"

typedef enum RenderScaleMode {
    RENDER_SCALE_ILLUSTRATIVE,
    RENDER_SCALE_REAL
} RenderScaleMode;

const char *renderer_scale_mode_label(RenderScaleMode mode);
Vec3d renderer_body_position(const SolarSystem *system, size_t body_index, RenderScaleMode mode);
Vec3d renderer_trail_point_position(const SolarSystem *system, const BodyTrails *trails, size_t body_index, size_t point_index, RenderScaleMode mode);
float renderer_body_radius(const Body *body, RenderScaleMode mode);
int renderer_grid_slices_for_system(const SolarSystem *system, RenderScaleMode mode);
void renderer_draw_solar_system(const SolarSystem *system, const BodyTrails *trails, RenderScaleMode mode);

#endif
