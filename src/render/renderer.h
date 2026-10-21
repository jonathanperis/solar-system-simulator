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
    int loc_occluders, loc_occluder_count, loc_sun_radius;
    Mesh sphere_detailed, sphere_simple, ring;
    Material material;
    Texture2D textures[RENDER_TEXTURE_COUNT];
    bool texture_loaded[RENDER_TEXTURE_COUNT];
    Texture2D white, glow;
    Font label_font;
    bool label_font_ready;
    bool ready;
} RenderResources;

/* Per-frame view: the raylib camera (in origin-relative render units), the
 * spin-model clock in TDB days since J2000, the far clip distance, whether
 * the reference grid and orbit guides are shown, and the selected body (its
 * guide is drawn brighter). */
typedef struct RenderView {
    Camera3D camera;
    double orientation_days;
    float far_plane;
    bool show_grid;
    bool show_orbits;
    size_t selected;
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
/* Per-frame lookups the trail drawer would otherwise repeat for every stored
 * sample: each body's parent index (a scan of the body list) and its drawn
 * position. Build once per frame; renderer_trail_point_cached returns exactly
 * what renderer_trail_point_in_frame would. */
typedef struct RenderFrameCache {
    int parent[SOLAR_SYSTEM_BODY_CAPACITY];
    Vec3d position[SOLAR_SYSTEM_BODY_CAPACITY];
} RenderFrameCache;
void renderer_frame_cache_build(RenderFrameCache *cache, const SolarSystem *system, RenderScaleMode mode);
Vec3d renderer_trail_point_cached(const RenderFrameCache *cache, const SolarSystem *system, const BodyTrails *trails,
    size_t body_index, size_t point_index, RenderScaleMode mode, RenderTrailFrame frame);
Vec3d renderer_vector_tip(const SolarSystem *system, size_t body_index, RenderScaleMode mode, bool acceleration);
double renderer_radius_magnification(const Body *body, RenderScaleMode mode);
void renderer_draw_vectors(const SolarSystem *system, size_t selected, RenderScaleMode mode, Vec3d origin);
size_t renderer_trail_sample_stride(size_t point_count);
size_t renderer_trail_draw_segment_count(size_t point_count);
float renderer_body_radius(const Body *body, RenderScaleMode mode);
float renderer_body_visual_radius(const Body *body, RenderScaleMode mode);
Vector3 renderer_relative_vector(Vec3d position, Vec3d origin);
/* Name labels in screen space (call after EndMode3D): the Sun, planets, the
 * selected body, and moons of a zoomed-in family, decluttered by priority. */
/* Backing pixels per CSS pixel (1 natively; the browser's devicePixelRatio,
 * capped at 2). Presentation only. */
void renderer_set_pixel_scale(float scale);
/* Screen-space markers after the 3D pass (SPEC A108): rings around dot-sized
 * planets, corner brackets on the selected body and a ring on the body under
 * the pointer (`hovered`, -1 for none; A118). Presentation only. */
void renderer_draw_markers(const SolarSystem *system, RenderScaleMode mode, Vec3d origin,
    const RenderResources *resources, Camera3D camera, size_t selected, int hovered);
/* The body whose label or off-screen marker (A119) covers a screen point in
 * the last drawn frame, or -1. Clicks try this before renderer_pick_body. */
int renderer_pick_target(Vector2 pointer);
/* Rectangles (x, y, width, height in screen pixels) of page panels floating
 * over the canvas; off-screen markers slide past them (A119). */
#define RENDER_MAX_BLOCKERS 12
void renderer_set_blockers(const float *rects, int count);
/* The body under a screen point (click to select, SPEC A112), or -1. */
int renderer_pick_body(const SolarSystem *system, RenderScaleMode mode, Vec3d origin, Camera3D camera, Vector2 pointer);
/* Body names. With `all` false (labels hidden) only the hovered body is
 * named; the hovered body is always named, like the selection (A118). */
void renderer_draw_labels(const SolarSystem *system, RenderScaleMode mode, Vec3d origin,
    const RenderResources *resources, Camera3D camera, size_t selected, int hovered, bool all);
void renderer_draw_solar_system(const SolarSystem *system, const BodyTrails *trails, RenderScaleMode mode,
    RenderTrailFrame trail_frame, Vec3d origin, const RenderResources *resources, const RenderView *view);

#endif
