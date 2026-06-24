#ifndef SOLAR_RENDERER_H
#define SOLAR_RENDERER_H

#include <stddef.h>

#include <raylib.h>

#include "../app/body_trails.h"
#include "../sim/solar_system.h"

#define SOLAR_RENDER_MAX_TRAIL_SEGMENTS 4096
#define SOLAR_RENDER_STAR_COUNT 96

typedef struct RendererStar {
    float x_norm;
    float y_norm;
    float radius;
    float alpha;
} RendererStar;

typedef struct RendererBodyStyle {
    Color base;
    Color accent;
    Color rim;
    Color glow;
    int halo_layers;
} RendererBodyStyle;

typedef enum RenderScaleMode {
    RENDER_SCALE_ILLUSTRATIVE,
    RENDER_SCALE_REAL
} RenderScaleMode;

const char *renderer_scale_mode_label(RenderScaleMode mode);
Color renderer_body_color(const Body *body);
RendererBodyStyle renderer_body_style(const Body *body);
RendererStar renderer_starfield_star(size_t index, double elapsed_seconds);
float renderer_trail_segment_alpha(size_t segment_index, size_t segment_count);
Vec3d renderer_body_position(const SolarSystem *system, size_t body_index, RenderScaleMode mode);
Vec3d renderer_trail_point_position(const SolarSystem *system, const BodyTrails *trails, size_t body_index, size_t point_index, RenderScaleMode mode);
size_t renderer_trail_sample_stride(size_t point_count);
size_t renderer_trail_draw_segment_count(size_t point_count);
float renderer_body_radius(const Body *body, RenderScaleMode mode);
int renderer_grid_slices_for_system(const SolarSystem *system, RenderScaleMode mode);
void renderer_draw_backdrop(int screen_width, int screen_height, double elapsed_seconds);
void renderer_draw_solar_system(const SolarSystem *system, const BodyTrails *trails, RenderScaleMode mode);

#endif
