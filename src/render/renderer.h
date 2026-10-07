#ifndef SOLAR_RENDERER_H
#define SOLAR_RENDERER_H

#include <stddef.h>

#include <raylib.h>

#include "../app/body_trails.h"
#include "../sim/solar_system.h"
#include "scene_style.h"

#define SOLAR_RENDER_MAX_TRAIL_SEGMENTS 1024

typedef enum RenderScaleMode {
    RENDER_SCALE_ILLUSTRATIVE,
    RENDER_SCALE_REAL
} RenderScaleMode;

typedef enum RenderTrailFrame { RENDER_TRAILS_ABSOLUTE, RENDER_TRAILS_PARENT } RenderTrailFrame;

typedef struct RenderSystemFrame {
    size_t root_index;
    double radius;
} RenderSystemFrame;

/* GPU resources for the cinematic renderer (SPEC A64-A67). Create them after
 * InitWindow (they need a GL context) and unload them before CloseWindow.
 * Every texture is optional: a missing map falls back to a lit body colour. */
typedef struct RenderResources {
    Shader shader;
    int loc_mode, loc_light_dir, loc_view_pos, loc_atmosphere, loc_night_lights;
    int loc_body_center, loc_body_radius, loc_ring_normal, loc_ring_radii, loc_ring_shadow;
    Mesh sphere_detailed, sphere_simple, ring;
    Material material;
    Texture2D textures[RENDER_TEXTURE_COUNT];
    bool texture_loaded[RENDER_TEXTURE_COUNT];
    Texture2D white, glow;
    bool ready;
} RenderResources;

/* Per-frame view: the raylib camera (in origin-relative render units), the
 * spin-model clock in TDB days since J2000, and the far clip distance. */
typedef struct RenderView {
    Camera3D camera;
    double orientation_days;
    float far_plane;
} RenderView;

bool renderer_resources_init(RenderResources *resources);
void renderer_resources_unload(RenderResources *resources);
/* Decode bundled JPEG/PNG bytes into a texture slot; false keeps the fallback. */
bool renderer_load_texture_memory(RenderResources *resources, RenderTextureSlot slot,
    const unsigned char *bytes, size_t length);
/* Load every slot from `directory`; returns how many textures loaded. */
int renderer_load_textures_from_directory(RenderResources *resources, const char *directory);
int renderer_loaded_texture_count(const RenderResources *resources);

RenderSystemFrame renderer_system_frame(const SolarSystem *system, size_t selected, RenderScaleMode mode);
RenderSystemFrame renderer_body_frame(const SolarSystem *system, size_t selected, RenderScaleMode mode);

const char *renderer_scale_mode_label(RenderScaleMode mode);
Color renderer_body_color(const Body *body);
Vec3d renderer_body_position(const SolarSystem *system, size_t body_index, RenderScaleMode mode);
Vec3d renderer_trail_point_position(const SolarSystem *system, const BodyTrails *trails, size_t body_index, size_t point_index, RenderScaleMode mode);
Vec3d renderer_trail_point_in_frame(const SolarSystem *system, const BodyTrails *trails, size_t body_index,
    size_t point_index, RenderScaleMode mode, RenderTrailFrame frame);
Vec3d renderer_vector_tip(const SolarSystem *system, size_t body_index, RenderScaleMode mode, bool acceleration);
double renderer_radius_magnification(const Body *body, RenderScaleMode mode);
void renderer_draw_vectors(const SolarSystem *system, size_t selected, RenderScaleMode mode, Vec3d origin);
size_t renderer_trail_sample_stride(size_t point_count);
size_t renderer_trail_draw_segment_count(size_t point_count);
float renderer_body_radius(const Body *body, RenderScaleMode mode);
float renderer_body_visual_radius(const Body *body, RenderScaleMode mode);
Vector3 renderer_relative_vector(Vec3d position, Vec3d origin);
void renderer_draw_solar_system(const SolarSystem *system, const BodyTrails *trails, RenderScaleMode mode,
    RenderTrailFrame trail_frame, Vec3d origin, const RenderResources *resources, const RenderView *view);

#endif
