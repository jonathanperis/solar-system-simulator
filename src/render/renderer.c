#include "renderer.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <rlgl.h>

#include "../sim/constants.h"
#include "render_scale.h"

#define SOLAR_ILLUSTRATIVE_SATELLITE_GAP_UNITS 0.03
#define SOLAR_ILLUSTRATIVE_SMALL_MOON_RADIUS 0.012f

/* Backing pixels per CSS pixel (HiDPI, SPEC A114). Detail budgets, size
 * thresholds and 2D marks are designed in CSS pixels: budgets divide the
 * viewport by this scale, drawn text and marks multiply by it. */
static float pixel_scale = 1.0f;
/* View from Earth (A143): the body the camera sits inside. It is not drawn,
 * labelled, picked or counted as an occluder; -1 when there is none. */
static int observer_body = -1;

void renderer_set_observer(int index)
{
    observer_body = index;
}

void renderer_set_pixel_scale(float scale)
{
    pixel_scale = scale >= 1.0f && scale <= 4.0f ? scale : 1.0f;
}

/* Viewport height in CSS pixels, for every pixel-based budget. */
static double css_viewport_height(void)
{
    return (double)GetRenderHeight() / pixel_scale;
}

static double render_aspect(void)
{
    return (double)GetRenderWidth() / (double)GetRenderHeight();
}

Vector3 renderer_relative_vector(Vec3d position, Vec3d origin)
{
    Vec3d vector = vec3d_sub(position, origin);
    return (Vector3){
        (float)vector.x,
        (float)vector.y,
        (float)vector.z,
    };
}

static float body_radius_raw(const Body *body, RenderScaleMode mode)
{
    /* Unknown radii use an explicitly nonphysical wire marker in either view. */
    if (body->radius_quality == PHYSICAL_UNKNOWN) return SOLAR_ILLUSTRATIVE_SMALL_MOON_RADIUS;
    float scaled_radius = meters_to_render_units(body->radius_m);
    if (mode == RENDER_SCALE_REAL) {
        return scaled_radius;
    }

    if (body->kind == BODY_KIND_STAR) {
        return scaled_radius < SOLAR_MIN_VISIBLE_BODY_RADIUS ? SOLAR_MIN_VISIBLE_BODY_RADIUS : scaled_radius;
    }

    if (body->kind == BODY_KIND_MOON) {
        /* Moons remain visible but smaller than planets; this is a visual
         * compromise only, while RENDER_SCALE_REAL keeps physical radii. */
        float moon_radius = SOLAR_ILLUSTRATIVE_PLANET_RADIUS * (float)(body->radius_m / SOLAR_EARTH_RADIUS_M);
        return moon_radius < SOLAR_ILLUSTRATIVE_SMALL_MOON_RADIUS ? SOLAR_ILLUSTRATIVE_SMALL_MOON_RADIUS : moon_radius;
    }

    if (body->kind == BODY_KIND_ASTEROID) {
        return SOLAR_ILLUSTRATIVE_ASTEROID_RADIUS;
    }

    if (body->kind == BODY_KIND_DWARF_PLANET) {
        /* Between an asteroid marker and a planet: Pluto (0.19 Earth radii)
         * reads as smaller than any planet but larger than Vesta. */
        float dwarf_radius = SOLAR_ILLUSTRATIVE_PLANET_RADIUS * (float)(body->radius_m / SOLAR_EARTH_RADIUS_M);
        return dwarf_radius < SOLAR_ILLUSTRATIVE_ASTEROID_RADIUS * 1.5f ? SOLAR_ILLUSTRATIVE_ASTEROID_RADIUS * 1.5f : dwarf_radius;
    }

    return SOLAR_ILLUSTRATIVE_PLANET_RADIUS;
}

/* Outer edge of a planet's drawn rings in metres, or 0 for no rings. */
static double ring_outer_radius_m(BodyId id)
{
    if (id == BODY_ID_SATURN) return SOLAR_SATURN_RING_OUTER_RADIUS_M;
    const RenderRingSystem *system = render_ring_system(id);
    return system ? system->extent_m : 0;
}

static float visual_radius_from(const Body *body, RenderScaleMode mode, float body_radius);

float renderer_body_visual_radius(const Body *body, RenderScaleMode mode)
{
    return visual_radius_from(body, mode, renderer_body_radius(body, mode));
}

static float visual_radius_raw(const Body *body, RenderScaleMode mode)
{
    return visual_radius_from(body, mode, body_radius_raw(body, mode));
}

static float visual_radius_from(const Body *body, RenderScaleMode mode, float body_radius)
{
    (void)mode;
    double rings = ring_outer_radius_m(body->id);
    if (rings <= 0) return body_radius;
    /* The rings scale with the drawn planet, so they follow a scale morph;
     * in real scale this is exactly their physical outer radius. */
    return body_radius * (float)(rings / body->radius_m);
}

/* How far from its parent's centre a satellite must be drawn so the two
 * drawn spheres (plus a gap) do not touch. A moon that really orbits outside
 * Saturn's rings also clears the drawn rings, so the enlarged ring plane never
 * slices through it (SPEC A115); ring-gap moons such as Pan keep the planet's
 * radius. `true_distance` is the undisplaced distance in render units. */
static double satellite_clearance(const Body *body, const Body *parent, double true_distance, RenderScaleMode mode)
{
    double parent_extent = body_radius_raw(parent, mode);
    /* The ring edge the moon really lies beyond: all drawn rings, or (for a
     * moon inside Jupiter's gossamer rings, made of its own dust) the main
     * rings that set the framing extent. */
    double rings = ring_outer_radius_m(parent->id);
    const RenderRingSystem *table = render_ring_system(parent->id);
    double drawn_outer = table ? table->outer_m : rings;
    if (drawn_outer > 0 && true_distance > meters_to_render_units(drawn_outer * RENDER_RING_CLEARANCE_MARGIN))
        parent_extent = body_radius_raw(parent, mode) * drawn_outer / parent->radius_m;
    else if (rings > 0 && true_distance > meters_to_render_units(rings))
        parent_extent = visual_radius_raw(parent, mode);
    return parent_extent + body_radius_raw(body, mode) + SOLAR_ILLUSTRATIVE_SATELLITE_GAP_UNITS;
}

static Vec3d visible_satellite_position(const Body *body, const Body *parent, Vec3d position, Vec3d parent_position, RenderScaleMode mode)
{
    Vec3d relative_position = vec3d_sub(position, parent_position);
    double current_distance = vec3d_length(relative_position);
    if (current_distance <= 0.0) {
        return position;
    }

    double required_distance = satellite_clearance(body, parent, current_distance, mode);

    if (body->id == BODY_ID_MOON) {
        Vec3d expanded = vec3d_scale(relative_position, SOLAR_ILLUSTRATIVE_MOON_DISTANCE_FACTOR);
        if (vec3d_length(expanded) > required_distance) {
            return vec3d_add(parent_position, expanded);
        }
    }

    /* Push every moon out by the clearance rather than onto it (A135):
     * clamping put all moons inside the enlarged planet on one shell, so
     * Europa could be drawn inside Io. Adding the clearance keeps their order
     * and their true spacing; for distant moons it is a small offset. */
    return vec3d_add(parent_position, vec3d_scale(relative_position, (current_distance + required_distance) / current_distance));
}

static Vec3d body_position_raw(const SolarSystem *system, size_t body_index, RenderScaleMode mode)
{
    if (system->body_count == 0 || body_index >= system->body_count) {
        return vec3d_zero();
    }

    const Body *body = &system->bodies[body_index];
    Vec3d position = meters_vec_to_render_vec3d(body->position_m);
    if (mode == RENDER_SCALE_REAL) {
        return position;
    }

    int parent_index = solar_system_parent_index(system, body_index);
    if (parent_index < 0) {
        return position;
    }

    const Body *parent = &system->bodies[parent_index];
    if (body->kind != BODY_KIND_MOON && parent->kind == BODY_KIND_STAR) return position;
    Vec3d parent_position = meters_vec_to_render_vec3d(parent->position_m);
    return visible_satellite_position(body, parent, position, parent_position, mode);
}

/* Trail sample position with the parent already resolved (-1 = none). */
static inline Vec3d trail_point_raw(const SolarSystem *system, const BodyTrails *trails, size_t body_index,
    size_t point_index, RenderScaleMode mode, int parent_index)
{
    if (system->body_count == 0 || body_index >= system->body_count || body_index >= SOLAR_SYSTEM_BODY_CAPACITY) {
        return vec3d_zero();
    }

    Vec3d position = meters_vec_to_render_vec3d(body_trails_point_at(trails, body_index, point_index));
    const Body *body = &system->bodies[body_index];
    if (mode == RENDER_SCALE_REAL) {
        return position;
    }

    if (parent_index < 0 || point_index >= body_trails_point_count(trails, (size_t)parent_index)) {
        return position;
    }

    const Body *parent = &system->bodies[parent_index];
    if (body->kind != BODY_KIND_MOON && parent->kind == BODY_KIND_STAR) return position;
    Vec3d parent_position = meters_vec_to_render_vec3d(body_trails_point_at(trails, (size_t)parent_index, point_index));
    return visible_satellite_position(body, parent, position, parent_position, mode);
}

/* Scale morph (SPEC A125). While the view switches scale, sizes and places
 * asked for in the new mode blend from the old mode's (`progress` 0 -> 1,
 * eased by the caller): radii in log space, since the two scales differ by
 * factors up to thousands, positions linearly. Presentation only; outside a
 * morph (progress 1) every mode draws exactly as before. */
static RenderScaleMode morph_from = RENDER_SCALE_ILLUSTRATIVE;
static float morph_progress = 1.0f;

void renderer_set_scale_morph(RenderScaleMode from, float progress)
{
    morph_from = from;
    morph_progress = progress < 0 ? 0 : progress > 1 ? 1 : progress;
}

static inline bool morphing(RenderScaleMode mode)
{
    return morph_progress < 1.0f && mode != morph_from;
}

static Vec3d blend_place(Vec3d from, Vec3d to)
{
    return vec3d_add(from, vec3d_scale(vec3d_sub(to, from), morph_progress));
}

float renderer_body_radius(const Body *body, RenderScaleMode mode)
{
    float to = body_radius_raw(body, mode);
    if (!morphing(mode)) return to;
    float from = body_radius_raw(body, morph_from);
    if (morph_progress <= 0.0f) return from;
    return from > 0 && to > 0 ? expf(logf(from) + (logf(to) - logf(from)) * morph_progress) : to;
}

Vec3d renderer_body_position(const SolarSystem *system, size_t body_index, RenderScaleMode mode)
{
    Vec3d to = body_position_raw(system, body_index, mode);
    if (!morphing(mode)) return to;
    Vec3d from = body_position_raw(system, body_index, morph_from);
    return morph_progress <= 0.0f ? from : blend_place(from, to);
}

static inline Vec3d trail_point_with_parent(const SolarSystem *system, const BodyTrails *trails, size_t body_index,
    size_t point_index, RenderScaleMode mode, int parent_index)
{
    /* Hot path (every trail sample, every frame): no morph, original code. */
    if (!morphing(mode)) return trail_point_raw(system, trails, body_index, point_index, mode, parent_index);
    Vec3d to = trail_point_raw(system, trails, body_index, point_index, mode, parent_index);
    Vec3d from = trail_point_raw(system, trails, body_index, point_index, morph_from, parent_index);
    return morph_progress <= 0.0f ? from : blend_place(from, to);
}

Vec3d renderer_trail_point_position(const SolarSystem *system, const BodyTrails *trails, size_t body_index, size_t point_index, RenderScaleMode mode)
{
    int parent = body_index < system->body_count ? solar_system_parent_index(system, body_index) : -1;
    return trail_point_with_parent(system, trails, body_index, point_index, mode, parent);
}

void renderer_frame_cache_build(RenderFrameCache *cache, const SolarSystem *system, RenderScaleMode mode)
{
    for (size_t i = 0; i < system->body_count && i < SOLAR_SYSTEM_BODY_CAPACITY; ++i) {
        cache->parent[i] = solar_system_parent_index(system, i);
        cache->position[i] = renderer_body_position(system, i, mode);
    }
}

Vec3d renderer_trail_point_cached(const RenderFrameCache *cache, const SolarSystem *system, const BodyTrails *trails,
    size_t body_index, size_t point_index, RenderScaleMode mode, RenderTrailFrame frame)
{
    if (body_index >= system->body_count) return vec3d_zero();
    int parent = cache->parent[body_index];
    Vec3d position = trail_point_with_parent(system, trails, body_index, point_index, mode, parent);
    if (frame == RENDER_TRAILS_ABSOLUTE || parent < 0 || system->bodies[parent].fixed) return position;
    /* Same parent-relative translation as renderer_trail_point_in_frame. */
    Vec3d then = trail_point_with_parent(system, trails, (size_t)parent, point_index, mode, cache->parent[parent]);
    return vec3d_add(vec3d_sub(position, then), cache->position[parent]);
}

RenderSystemFrame renderer_system_frame(const SolarSystem *system, size_t selected, RenderScaleMode mode)
{
    RenderSystemFrame frame = {.root_index = selected};
    int parent = solar_system_parent_index(system, selected);
    if (parent >= 0 && system->bodies[parent].kind != BODY_KIND_STAR) frame.root_index = (size_t)parent;
    const Body *root = &system->bodies[frame.root_index];
    Vec3d center = renderer_body_position(system, frame.root_index, mode);
    for (size_t i = 0; i < system->body_count; ++i) {
        if (root->parent_id == BODY_ID_NONE || i == frame.root_index || system->bodies[i].parent_id == root->id) {
            double extent = vec3d_length(vec3d_sub(renderer_body_position(system, i, mode), center))
                + renderer_body_visual_radius(&system->bodies[i], mode);
            frame.radius = fmax(frame.radius, extent);
        }
    }
    return frame;
}

Vec3d renderer_trail_point_in_frame(const SolarSystem *system, const BodyTrails *trails, size_t body_index,
    size_t point_index, RenderScaleMode mode, RenderTrailFrame frame)
{
    Vec3d position = renderer_trail_point_position(system, trails, body_index, point_index, mode);
    int parent = solar_system_parent_index(system, body_index);
    if (frame == RENDER_TRAILS_ABSOLUTE || parent < 0 || system->bodies[parent].fixed) return position;
    /* Historical parent and child samples share times. Translate the old pair
     * to today's parent location to expose the relative orbit, never its SI state. */
    Vec3d then = renderer_trail_point_position(system, trails, (size_t)parent, point_index, mode);
    Vec3d now = renderer_body_position(system, (size_t)parent, mode);
    return vec3d_add(vec3d_sub(position, then), now);
}

double renderer_radius_magnification(const Body *body, RenderScaleMode mode)
{
    return body->radius_quality == PHYSICAL_UNKNOWN || body->radius_m <= 0 ? 0
        : (double)body_radius_raw(body, mode) / meters_to_render_units(body->radius_m);
}

Vec3d renderer_vector_tip(const SolarSystem *system, size_t body_index, RenderScaleMode mode, bool acceleration)
{
    const Body *body = &system->bodies[body_index];
    Vec3d direction = acceleration ? body->acceleration_mps2 : body->velocity_mps;
    if (body->fixed) direction = vec3d_zero();
    int parent = solar_system_parent_index(system, body_index);
    if (parent >= 0 && !system->bodies[parent].fixed) direction = vec3d_sub(direction,
        acceleration ? system->bodies[parent].acceleration_mps2 : system->bodies[parent].velocity_mps);
    double length = vec3d_length(direction);
    Vec3d start = renderer_body_position(system, body_index, mode);
    return length > 0 ? vec3d_add(start, vec3d_scale(direction, 4.0 * renderer_body_radius(body, mode) / length)) : start;
}

void renderer_draw_vectors(const SolarSystem *system, size_t selected, RenderScaleMode mode, Vec3d origin)
{
    Vector3 start = renderer_relative_vector(renderer_body_position(system, selected, mode), origin);
    for (int acceleration = 0; acceleration < 2; ++acceleration) {
        Vector3 end = renderer_relative_vector(renderer_vector_tip(system, selected, mode, acceleration != 0), origin);
        Color color = acceleration ? ORANGE : GREEN;
        DrawLine3D(start, end, color);
        DrawSphere(end, renderer_body_radius(&system->bodies[selected], mode) * 0.15f, color);
    }
}

RenderSystemFrame renderer_body_frame(const SolarSystem *system, size_t selected, RenderScaleMode mode)
{
    return (RenderSystemFrame){selected, renderer_body_visual_radius(&system->bodies[selected], mode)};
}

Color renderer_body_color(const Body *body)
{
    switch (body->id) {
        case BODY_ID_SUN:
            return GOLD;
        case BODY_ID_MERCURY:
            return GRAY;
        case BODY_ID_VENUS:
            return BEIGE;
        case BODY_ID_EARTH:
            return BLUE;
        case BODY_ID_MOON:
            return RAYWHITE;
        case BODY_ID_MARS:
            return ORANGE;
        case BODY_ID_PHOBOS:
            return BROWN;
        case BODY_ID_DEIMOS:
            return MAROON;
        case BODY_ID_VESTA:
            return LIGHTGRAY;
        case BODY_ID_JUPITER:
            return (Color){206, 164, 118, 255};
        case BODY_ID_SATURN:
            return (Color){218, 190, 130, 255};
        case BODY_ID_URANUS: return (Color){139, 211, 220, 255};
        case BODY_ID_NEPTUNE: return (Color){72, 113, 216, 255};
        case BODY_ID_IO: return (Color){230, 205, 94, 255};
        case BODY_ID_EUROPA: return (Color){205, 215, 223, 255};
        case BODY_ID_GANYMEDE: return (Color){164, 152, 129, 255};
        case BODY_ID_CALLISTO: return (Color){130, 145, 160, 255};
        /* Main-scene moons without texture maps keep honest flat colours:
         * Titan's orange haze, Enceladus's bright ice, Triton's pinkish frost. */
        case BODY_ID_TITAN: return (Color){214, 168, 92, 255};
        case BODY_ID_ENCELADUS: return (Color){236, 242, 248, 255};
        case BODY_ID_TRITON: return (Color){214, 196, 186, 255};
        case BODY_ID_PLUTO: return (Color){214, 186, 150, 255};
        case BODY_ID_CHARON: return (Color){168, 164, 160, 255};
        case BODY_ID_DIDYMOS: return (Color){150, 138, 124, 255};
        case BODY_ID_DIMORPHOS: return (Color){172, 160, 146, 255};
        /* Dark P-type trojans (geometric albedo 0.047): a dim reddish grey. */
        case BODY_ID_PATROCLUS: return (Color){128, 112, 100, 255};
        case BODY_ID_MENOETIUS: return (Color){144, 128, 116, 255};
        case BODY_ID_UNKNOWN:
        case BODY_ID_NONE:
        default:
            break;
    }

    if (body->kind == BODY_KIND_STAR) {
        return GOLD;
    }
    return LIGHTGRAY;
}

const char *renderer_scale_mode_label(RenderScaleMode mode)
{
    switch (mode) {
        case RENDER_SCALE_REAL:
            return "Real scale";
        case RENDER_SCALE_ILLUSTRATIVE:
        default:
            return "Illustrative";
    }
}

size_t renderer_trail_sample_stride(size_t point_count)
{
    if (point_count <= SOLAR_RENDER_MAX_TRAIL_SEGMENTS + 1) {
        return 1;
    }

    size_t segment_count = point_count - 1;
    return (segment_count + SOLAR_RENDER_MAX_TRAIL_SEGMENTS - 2) / (SOLAR_RENDER_MAX_TRAIL_SEGMENTS - 1);
}

size_t renderer_trail_draw_segment_count(size_t point_count)
{
    if (point_count < 2) {
        return 0;
    }

    size_t stride = renderer_trail_sample_stride(point_count);
    size_t segment_count = point_count - 1;
    size_t drawn_segments = segment_count / stride;
    if ((segment_count % stride) != 0) {
        ++drawn_segments;
    }
    return drawn_segments;
}

/* Body-fixed frame -> world: column 0 is the prime meridian (longitude 0),
 * column 1 the north pole, column 2 their cross product, each scaled by the
 * drawn radius, then translated. raylib matrices are column-major: m0-m2 hold
 * column 0, m12-m14 the translation. */
static Matrix body_matrix(RenderOrientation orientation, Vector3 position, float radius)
{
    Vec3d x = orientation.prime_meridian, y = orientation.pole, z = vec3d_cross(x, y);
    Matrix m = {0};
    m.m0 = (float)x.x * radius; m.m1 = (float)x.y * radius; m.m2 = (float)x.z * radius;
    m.m4 = (float)y.x * radius; m.m5 = (float)y.y * radius; m.m6 = (float)y.z * radius;
    m.m8 = (float)z.x * radius; m.m9 = (float)z.y * radius; m.m10 = (float)z.z * radius;
    m.m12 = position.x; m.m13 = position.y; m.m14 = position.z; m.m15 = 1.0f;
    return m;
}

/* Saturn and its rings shadow each other (computed analytically in the
 * shader). Every other draw passes NULL and the shader skips the test. */
typedef struct RingShadow {
    Vector3 center, ring_normal;
    float radius, ring_inner, ring_outer; /* ring edges in planet radii */
    Texture2D ring_opacity;
    float dust; /* forward-scattering dust fraction (A127); Saturn's icy main rings 0.05 */
} RingShadow;

/* raylib allocates MAX_MATERIAL_MAPS (12, an internal config.h value) maps
 * per material and DrawMesh reads all of them, so a private copy needs 12. */
#define RAYLIB_MATERIAL_MAPS 12

typedef enum ShadeMode { SHADE_LIT = 0, SHADE_STAR = 1, SHADE_CLOUDS = 2, SHADE_RING = 3, SHADE_SKY = 4, SHADE_HALO = 5 } ShadeMode;

/* Set this draw's uniforms and maps, then issue it. DrawMesh draws at once
 * (it is not batched), so per-body uniforms take effect immediately. */
static void draw_shaded(const RenderResources *resources, const Mesh *mesh, const Matrix *transform, ShadeMode mode,
    Texture2D surface, Color tint, Vector3 light_direction, RenderAtmosphere atmosphere, const Texture2D *night,
    const RingShadow *rings)
{
    float ring_flag = rings ? 1.0f : 0.0f;
    SetShaderValue(resources->shader, resources->loc_ring_shadow, &ring_flag, SHADER_UNIFORM_FLOAT);
    if (rings) {
        float center[3] = {rings->center.x, rings->center.y, rings->center.z};
        float normal[3] = {rings->ring_normal.x, rings->ring_normal.y, rings->ring_normal.z};
        float radii[2] = {rings->ring_inner, rings->ring_outer};
        SetShaderValue(resources->shader, resources->loc_body_center, center, SHADER_UNIFORM_VEC3);
        SetShaderValue(resources->shader, resources->loc_body_radius, &rings->radius, SHADER_UNIFORM_FLOAT);
        SetShaderValue(resources->shader, resources->loc_ring_normal, normal, SHADER_UNIFORM_VEC3);
        SetShaderValue(resources->shader, resources->loc_ring_radii, radii, SHADER_UNIFORM_VEC2);
        if (resources->loc_ring_dust >= 0)
            SetShaderValue(resources->shader, resources->loc_ring_dust, &rings->dust, SHADER_UNIFORM_FLOAT);
    }
    float mode_value = (float)mode, night_value = night ? 1.0f : 0.0f;
    float light[3] = {light_direction.x, light_direction.y, light_direction.z};
    float air[4] = {atmosphere.r, atmosphere.g, atmosphere.b, atmosphere.strength};
    SetShaderValue(resources->shader, resources->loc_mode, &mode_value, SHADER_UNIFORM_FLOAT);
    SetShaderValue(resources->shader, resources->loc_light_dir, light, SHADER_UNIFORM_VEC3);
    SetShaderValue(resources->shader, resources->loc_atmosphere, air, SHADER_UNIFORM_VEC4);
    SetShaderValue(resources->shader, resources->loc_night_lights, &night_value, SHADER_UNIFORM_FLOAT);
    /* Work on a private copy of the map array so the shared material in
     * RenderResources stays untouched (it is const here for a reason). */
    MaterialMap maps[RAYLIB_MATERIAL_MAPS];
    memcpy(maps, resources->material.maps, sizeof(maps));
    Material material = resources->material;
    material.maps = maps;
    material.maps[MATERIAL_MAP_ALBEDO].texture = surface;
    material.maps[MATERIAL_MAP_ALBEDO].color = tint;
    material.maps[MATERIAL_MAP_METALNESS].texture = night ? *night : resources->white;
    material.maps[MATERIAL_MAP_NORMAL].texture = rings ? rings->ring_opacity : resources->white;
    DrawMesh(*mesh, material, *transform);
}

/* Ring geometry for the shadow tests, matching the ring mesh (radii in planet
 * radii, plane normal = Saturn's pole). Always filled; returns whether the
 * ring opacity map exists, which only the rings' shadow on Saturn needs. */
/* Index of a planet's table-driven ring system, or -1 (A122, A126). */
static int ring_system_index(BodyId id)
{
    for (size_t k = 0; k < RENDER_RING_SYSTEM_COUNT; ++k) if (render_ring_system_at(k)->planet == id) return (int)k;
    return -1;
}

static Texture2D ring_strip(const RenderResources *resources, int system, RenderScaleMode mode)
{
    return mode == RENDER_SCALE_REAL ? resources->ring_strips[system] : resources->ring_strips_illustrative[system];
}

static bool planet_ring_shadow(const RenderResources *resources, BodyId id, RenderOrientation orientation, Vector3 position,
    float radius, RenderScaleMode mode, RingShadow *out)
{
    int faint = ring_system_index(id);
    const RenderRingSystem *table = faint >= 0 ? render_ring_system_at((size_t)faint) : NULL;
    bool ring_map = table ? ring_strip(resources, faint, mode).id != 0 : resources->texture_loaded[RENDER_TEXTURE_SATURN_RING];
    *out = (RingShadow){
        .center = position,
        .ring_normal = {(float)orientation.pole.x, (float)orientation.pole.y, (float)orientation.pole.z},
        .radius = radius,
        .ring_inner = table ? (float)(table->inner_m / table->planet_radius_m) : (float)SOLAR_SATURN_RING_VISUAL_INNER_RATIO,
        .ring_outer = table ? (float)(table->outer_m / table->planet_radius_m)
            : (float)(SOLAR_SATURN_RING_OUTER_RADIUS_M / SOLAR_SATURN_RADIUS_M),
        .ring_opacity = !ring_map ? resources->white : table ? ring_strip(resources, faint, mode) : resources->textures[RENDER_TEXTURE_SATURN_RING],
        .dust = table ? (float)table->dust_fraction : 0.05f,
    };
    return ring_map;
}

static Texture2D texture_or_white(const RenderResources *resources, RenderTextureSlot slot)
{
    return slot >= 0 && resources->texture_loaded[slot] ? resources->textures[slot] : resources->white;
}

static Vector3 unit_vector(Vector3 v, Vector3 fallback)
{
    float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    return length > 1e-20f ? (Vector3){v.x / length, v.y / length, v.z / length} : fallback;
}

static unsigned char line_alpha_byte(Color color, float alpha);

static void line_vertex(Vector3 position, Color color, float alpha)
{
    rlColor4ub(color.r, color.g, color.b, line_alpha_byte(color, alpha));
    rlVertex3f(position.x, position.y, position.z);
}

/* Adaptive ecliptic-plane reference grid (A67): power-of-ten minor and major
 * lines that cross-fade with zoom, each line split into short pieces so its
 * opacity can fall off with distance from the camera target. */
static unsigned char line_alpha_byte(Color color, float alpha)
{
    return (unsigned char)(color.a * (alpha < 0 ? 0 : alpha > 1 ? 1 : alpha));
}

static void draw_reference_grid(Vec3d origin, double camera_distance, const Camera3D *camera, double aspect)
{
    RenderGridLevels grid = render_grid_levels(camera_distance);
    /* The grid reaches 3.5 camera distances out and fades to nothing well
     * before that, so most of its pieces are transparent or off screen (about
     * 13k vertices a frame in the Saturn overview, as many as every trail).
     * A piece is skipped only when it cannot change a pixel: both endpoint
     * alphas round to zero (the GPU interpolates alpha linearly between
     * them), or both endpoints lie outside one side of the view. */
    Vec3d camera_world = vec3d_add(origin, (Vec3d){camera->position.x, camera->position.y, camera->position.z});
    RenderFrustum view = render_frustum(camera_world,
        (Vec3d){camera->target.x - camera->position.x, camera->target.y - camera->position.y, camera->target.z - camera->position.z},
        (Vec3d){camera->up.x, camera->up.y, camera->up.z}, camera->fovy, aspect, 0.05);
    const int pieces = 6;
    const Color minor_color = {70, 92, 118, 34}, major_color = {92, 120, 150, 72};
    rlBegin(RL_LINES);
    for (int level = 0; level < 2; ++level) {
        double spacing = level ? grid.major_spacing : grid.minor_spacing;
        double strength = level ? 1.0 : grid.minor_alpha;
        if (strength < 0.03) continue;
        Color color = level ? major_color : minor_color;
        long count = (long)ceil(grid.radius / spacing);
        double center_x = floor(origin.x / spacing) * spacing, center_z = floor(origin.z / spacing) * spacing;
        /* Slightly below the orbital plane so trails never share its depth. */
        double plane_y = -0.002 * grid.minor_spacing;
        for (long k = -count; k <= count; ++k) {
            for (int axis = 0; axis < 2; ++axis) {
                /* Minor lines that coincide with a major line are drawn once;
                 * x lines sit at center_x + k s, z lines at center_z + k s. */
                double center = axis == 0 ? center_x : center_z;
                if (!level && (llabs((long long)floor(center / spacing + 0.5) + k) % 10) == 0) continue;
                for (int piece = 0; piece < pieces; ++piece) {
                    double t0 = -grid.radius + 2 * grid.radius * piece / pieces;
                    double t1 = -grid.radius + 2 * grid.radius * (piece + 1) / pieces;
                    Vec3d a, b;
                    if (axis == 0) {
                        double x = center_x + k * spacing;
                        a = (Vec3d){x, plane_y, origin.z + t0}; b = (Vec3d){x, plane_y, origin.z + t1};
                    } else {
                        double z = center_z + k * spacing;
                        a = (Vec3d){origin.x + t0, plane_y, z}; b = (Vec3d){origin.x + t1, plane_y, z};
                    }
                    double da = hypot(a.x - origin.x, a.z - origin.z), db = hypot(b.x - origin.x, b.z - origin.z);
                    float alpha_a = (float)(strength * render_grid_alpha(da, grid.radius) * 2.0);
                    float alpha_b = (float)(strength * render_grid_alpha(db, grid.radius) * 2.0);
                    if (!line_alpha_byte(color, alpha_a) && !line_alpha_byte(color, alpha_b)) continue;
                    if (render_frustum_outcode(&view, a) & render_frustum_outcode(&view, b)) continue;
                    line_vertex(renderer_relative_vector(a, origin), color, alpha_a);
                    line_vertex(renderer_relative_vector(b, origin), color, alpha_b);
                }
            }
        }
    }
    rlEnd();
}

/* Approximate on-screen length (pixels) of a body's trail: the polyline
 * through up to 128 evenly spaced samples, measured at the distance of the
 * nearest one. Path length, not extent, so a moon that has looped many times
 * around a small region still gets enough points per orbit. Only used to
 * choose how finely to draw. */
static double trail_path_pixels(const RenderFrameCache *cache, const SolarSystem *system, const BodyTrails *trails,
    size_t body, size_t count, RenderScaleMode mode, RenderTrailFrame frame, Vec3d camera_world, const Camera3D *camera,
    double viewport_height)
{
    const size_t samples = count < 128 ? count : 128;
    Vec3d previous = renderer_trail_point_cached(cache, system, trails, body, 0, mode, frame);
    double length = 0, nearest = vec3d_length(vec3d_sub(previous, camera_world));
    for (size_t k = 1; k < samples; ++k) {
        Vec3d point = renderer_trail_point_cached(cache, system, trails, body, (count - 1) * k / (samples - 1), mode, frame);
        length += vec3d_length(vec3d_sub(point, previous));
        nearest = fmin(nearest, vec3d_length(vec3d_sub(point, camera_world)));
        previous = point;
    }
    double per_pixel = render_world_units_per_pixel(fmax(nearest, 1e-9), camera->fovy, viewport_height);
    return per_pixel > 0 ? length / per_pixel : 0;
}

/* `cache` holds this frame's parents and positions: trails visit up to ~130k
 * samples, so those lookups are resolved once per frame, not per sample. */
/* Measuring a trail's on-screen path length samples up to 128 points per
 * trail, the largest trail cost in a 300-body scene. Each trail is re-measured
 * every TRAIL_EXTENT_REFRESH_FRAMES frames, staggered across bodies, and at
 * once when its history shrinks (reset, new scene). Drawing detail can lag a
 * camera move by a few frames; it never drops below RENDER_TRAIL_MIN_POINTS. */
#define TRAIL_EXTENT_REFRESH_FRAMES 8
static double trail_extent_pixels[SOLAR_SYSTEM_BODY_CAPACITY];
static size_t trail_extent_points[SOLAR_SYSTEM_BODY_CAPACITY];
static unsigned trail_extent_frame;

static void draw_trails(const RenderFrameCache *cache, const SolarSystem *system, const BodyTrails *trails,
    RenderScaleMode mode, RenderTrailFrame trail_frame, Vec3d origin, const Camera3D *camera)
{
    Vec3d camera_world = vec3d_add(origin, (Vec3d){camera->position.x, camera->position.y, camera->position.z});
    double viewport_height = css_viewport_height();
    /* Segments wholly outside one side of the view are skipped: around a
     * framed moon, most of a distant moon's history lies off screen, and
     * submitting it vertex by vertex was the largest trail cost. */
    Vec3d forward = {camera->target.x - camera->position.x, camera->target.y - camera->position.y,
        camera->target.z - camera->position.z};
    RenderFrustum view = render_frustum(camera_world, forward, (Vec3d){camera->up.x, camera->up.y, camera->up.z},
        camera->fovy, render_aspect(), 0.05);
    ++trail_extent_frame;
    rlBegin(RL_LINES);
    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        if ((body->kind == BODY_KIND_STAR && body->fixed) || (int)i == observer_body) continue;
        size_t point_count = body_trails_point_count(trails, i);
        if (point_count < 2) continue;
        Color color = renderer_body_color(body);
        color.a = 235;
        /* Draw only as finely as the trail's on-screen length can show. */
        if ((i + trail_extent_frame) % TRAIL_EXTENT_REFRESH_FRAMES == 0 || point_count < trail_extent_points[i]) {
            trail_extent_pixels[i] = trail_path_pixels(cache, system, trails, i, point_count, mode, trail_frame,
                camera_world, camera, viewport_height);
            trail_extent_points[i] = point_count;
        }
        size_t stride = render_trail_stride_for_extent(point_count, renderer_trail_sample_stride(point_count),
            trail_extent_pixels[i]);
        /* The live endpoint is the body's centre: stop the trail at its drawn
         * surface so it never pokes out through the near side. */
        Vec3d center = cache->position[i];
        double surface = body->radius_quality == PHYSICAL_UNKNOWN ? 0 : renderer_body_radius(body, mode);
        /* Adjacent segments share an endpoint. Reuse its render transform;
         * the simulation and synchronized history are immutable while drawing.
         * Opacity follows the sample's age so recent motion reads first. */
        size_t previous = 0;
        Vec3d start = renderer_trail_point_cached(cache, system, trails, i, 0, mode, trail_frame);
        unsigned start_code = render_frustum_outcode(&view, start);
        for (size_t j = stride; j < point_count; j += stride) {
            Vec3d end = renderer_trail_point_cached(cache, system, trails, i, j, mode, trail_frame);
            unsigned end_code = render_frustum_outcode(&view, end);
            Vec3d a = start, b = end;
            if (!(start_code & end_code) && render_clip_segment_outside_sphere(&a, &b, center, surface)) {
                line_vertex(renderer_relative_vector(a, origin), color, render_trail_alpha(previous, point_count));
                line_vertex(renderer_relative_vector(b, origin), color, render_trail_alpha(j, point_count));
            }
            start = end;
            start_code = end_code;
            previous = j;
        }
        if (previous + 1 < point_count) {
            Vec3d end = renderer_trail_point_cached(cache, system, trails, i, point_count - 1, mode, trail_frame);
            Vec3d a = start, b = end;
            if (!(start_code & render_frustum_outcode(&view, end)) && render_clip_segment_outside_sphere(&a, &b, center, surface)) {
                line_vertex(renderer_relative_vector(a, origin), color, render_trail_alpha(previous, point_count));
                line_vertex(renderer_relative_vector(b, origin), color, 1.0f);
            }
        }
    }
    rlEnd();
}

/* Drawn position of a guide point: the parent's current place plus the
 * conic offset, then the same illustrative spreading as the body itself
 * (renderer_body_position), so a guide always passes through its body.
 * `clearance` >= 0 is that spreading's clearance when it is the same over
 * the whole orbit, saving the per-vertex lookup (identical result). */
static Vec3d orbit_guide_vertex(const SolarSystem *system, int parent_index, const Body *body, const Body *parent,
    Vec3d parent_render, Vec3d offset_m, RenderScaleMode mode, bool spread, double clearance)
{
    Vec3d position = meters_vec_to_render_vec3d(vec3d_add(parent->position_m, offset_m));
    if (morphing(mode)) {
        /* During a scale morph (A125) the bodies are drawn between the two
         * scales' places; the guide follows the same blend so it keeps
         * passing through its body (review fix). */
        Vec3d place[2];
        RenderScaleMode modes[2] = {morph_from, mode};
        for (int k = 0; k < 2; ++k) {
            bool spreads = modes[k] != RENDER_SCALE_REAL && !(body->kind != BODY_KIND_MOON && parent->kind == BODY_KIND_STAR);
            place[k] = spreads ? visible_satellite_position(body, parent, position,
                body_position_raw(system, (size_t)parent_index, modes[k]), modes[k]) : position;
        }
        return morph_progress <= 0.0f ? place[0] : blend_place(place[0], place[1]);
    }
    if (!spread) return position;
    if (clearance >= 0) {
        Vec3d relative = vec3d_sub(position, parent_render);
        double distance = vec3d_length(relative);
        if (distance > 0) return vec3d_add(parent_render, vec3d_scale(relative, (distance + clearance) / distance));
    }
    return visible_satellite_position(body, parent, position, parent_render, mode);
}

/* Screen rectangles that select a body when clicked (A119): visible labels
 * and off-screen markers, rebuilt every frame by the marker and label passes. */
#define CLICK_TARGET_CAPACITY (SOLAR_SYSTEM_BODY_CAPACITY + 64)
typedef struct ClickTarget {
    float x, y, width, height;
    int body;
} ClickTarget;
static ClickTarget click_targets[CLICK_TARGET_CAPACITY];
static size_t click_target_count;
/* One extra slot holds the scale bar's box (A123), set every frame. */
/* Page panels, then the scale bar, then last frame's placed labels (A151):
 * edge markers slide along the edge past all of them. */
#define RENDER_MAX_LABEL_BLOCKERS 24
static double blockers[RENDER_MAX_BLOCKERS + 1 + RENDER_MAX_LABEL_BLOCKERS][4];
static double placed_labels[RENDER_MAX_LABEL_BLOCKERS][4];
static int placed_label_count, panel_blocker_count;
static double caption_box[4]; /* the app's canvas caption, if any (A151) */

void renderer_set_caption_box(float x, float y, float width, float height)
{
    caption_box[0] = x; caption_box[1] = y; caption_box[2] = width; caption_box[3] = height;
}
static int blocker_count, page_blocker_count;

float renderer_caption_bottom(float x0, float x1, float bottom)
{
    return (float)render_clear_above((const double (*)[4])blockers, page_blocker_count, x0, x1, bottom);
}

void renderer_set_blockers(const float *rects, int count)
{
    page_blocker_count = count < 0 ? 0 : count > RENDER_MAX_BLOCKERS ? RENDER_MAX_BLOCKERS : count;
    blocker_count = page_blocker_count;
    for (int k = 0; k < blocker_count; ++k)
        for (int c = 0; c < 4; ++c) blockers[k][c] = rects[k * 4 + c];
}

static void add_click_target(float x, float y, float width, float height, size_t body)
{
    if (click_target_count < CLICK_TARGET_CAPACITY)
        click_targets[click_target_count++] = (ClickTarget){x, y, width, height, (int)body};
}

int renderer_pick_target(Vector2 pointer)
{
    const float pad = 3.0f * pixel_scale;
    for (size_t k = click_target_count; k-- > 0;) {
        const ClickTarget *t = &click_targets[k];
        if (pointer.x >= t->x - pad && pointer.x <= t->x + t->width + pad && pointer.y >= t->y - pad
            && pointer.y <= t->y + t->height + pad) return t->body;
    }
    return -1;
}

void renderer_draw_labels(const SolarSystem *system, RenderScaleMode mode, Vec3d origin,
    const RenderResources *resources, Camera3D camera, size_t selected, int hovered, bool all)
{
    static RenderLabelBox boxes[SOLAR_SYSTEM_BODY_CAPACITY];
    static size_t body_of[SOLAR_SYSTEM_BODY_CAPACITY];
    static Vector2 screen[SOLAR_SYSTEM_BODY_CAPACITY];
    static bool on_screen[SOLAR_SYSTEM_BODY_CAPACITY], in_front[SOLAR_SYSTEM_BODY_CAPACITY];
    static double occluder_distance[SOLAR_SYSTEM_BODY_CAPACITY], occluder_radius[SOLAR_SYSTEM_BODY_CAPACITY];
    const float s = pixel_scale, size = 15.0f * s, spacing = 0.5f * s;
    Font font = resources->label_font_ready ? resources->label_font : GetFontDefault();
    Vector3 forward = unit_vector((Vector3){camera.target.x - camera.position.x, camera.target.y - camera.position.y,
        camera.target.z - camera.position.z}, (Vector3){0, 0, -1});
    float width = (float)GetScreenWidth(), height = (float)GetScreenHeight();
    double viewport_height = (double)height, css_height = viewport_height / s;

    /* Project every body once; labels then only read the cached results. */
    for (size_t i = 0; i < system->body_count && i < SOLAR_SYSTEM_BODY_CAPACITY; ++i) {
        Vector3 position = renderer_relative_vector(renderer_body_position(system, i, mode), origin);
        Vector3 offset = {position.x - camera.position.x, position.y - camera.position.y, position.z - camera.position.z};
        in_front[i] = offset.x * forward.x + offset.y * forward.y + offset.z * forward.z > 0;
        if ((int)i == observer_body) in_front[i] = false;
        screen[i] = in_front[i] ? GetWorldToScreen(position, camera) : (Vector2){-1e6f, -1e6f};
        on_screen[i] = in_front[i] && screen[i].x >= 0 && screen[i].x <= width && screen[i].y >= 0 && screen[i].y <= height;
        occluder_distance[i] = sqrt(offset.x * offset.x + offset.y * offset.y + offset.z * offset.z);
        occluder_radius[i] = system->bodies[i].radius_quality == PHYSICAL_UNKNOWN || (int)i == observer_body ? 0
            : render_projected_radius_pixels(renderer_body_radius(&system->bodies[i], mode), occluder_distance[i], camera.fovy, viewport_height);
    }

    size_t count = 0;
    for (size_t i = 0; i < system->body_count && i < SOLAR_SYSTEM_BODY_CAPACITY; ++i) {
        if (!on_screen[i]) continue;
        bool is_hovered = (int)i == hovered;
        if (!all && !is_hovered) continue;
        const Body *body = &system->bodies[i];
        Vector3 position = renderer_relative_vector(renderer_body_position(system, i, mode), origin);
        Vector3 offset = {position.x - camera.position.x, position.y - camera.position.y, position.z - camera.position.z};
        double distance = sqrt(offset.x * offset.x + offset.y * offset.y + offset.z * offset.z);
        /* Label decisions use CSS pixels; occluders and boxes stay in screen pixels. */
        double radius_px = render_projected_radius_pixels(renderer_body_radius(body, mode), distance, camera.fovy, css_height);
        int parent = solar_system_parent_index(system, i);
        double separation = 1e9;
        if (parent >= 0 && system->bodies[parent].kind != BODY_KIND_STAR)
            separation = hypot(screen[i].x - screen[parent].x, screen[i].y - screen[parent].y) / s;
        /* From Earth (A149) the target's own moons are named through the
         * telescope even as points of light. */
        bool sky_moon = observer_body >= 0 && parent >= 0 && (size_t)parent == selected
            && render_sky_moon_wants_label(body->radius_quality != PHYSICAL_UNKNOWN, separation);
        if (!sky_moon && !render_body_wants_label(body->kind, i == selected || is_hovered, body->radius_quality != PHYSICAL_UNKNOWN, radius_px, separation))
            continue;
        /* Hidden behind a nearer body that is large on screen: no label. */
        bool hidden = false;
        for (size_t j = 0; j < system->body_count && !hidden; ++j) {
            /* A large nearer body can hide a label even when its own centre
             * is just off screen, so occluders only need to be in front. */
            if (j == i || !in_front[j] || occluder_radius[j] < 8.0 * s || occluder_distance[j] >= distance) continue;
            hidden = hypot(screen[i].x - screen[j].x, screen[i].y - screen[j].y) < occluder_radius[j];
        }
        if (hidden) continue;
        Vector2 text = MeasureTextEx(font, body->name, size, spacing);
        int priority = i == selected ? 1000 : is_hovered ? 950 : body->kind == BODY_KIND_STAR ? 900
            : body->kind == BODY_KIND_PLANET ? 800 - (int)i : body->kind == BODY_KIND_DWARF_PLANET ? 600 - (int)i : 400 - (int)i;
        /* Above the body (and its ring marker or selection brackets), but
         * kept on screen when the body fills the view. */
        double clearance = radius_px;
        if (render_body_wants_ring_marker(body->kind, radius_px)) clearance = fmax(clearance, RENDER_RING_MARKER_RADIUS_PIXELS);
        if (i == selected && render_selection_wants_brackets(radius_px))
            clearance = fmax(clearance, render_selection_bracket_half_size(radius_px));
        if (is_hovered) clearance = fmax(clearance, render_hover_ring_radius(radius_px));
        float top = fmaxf(4.0f * s, screen[i].y - (float)clearance * s - 4.0f * s - text.y);
        /* Kept inside the view sideways too: a moon near the edge keeps its
         * whole name (A151). */
        float left = fminf(fmaxf(4.0f * s, screen[i].x - text.x / 2), width - text.x - 4.0f * s);
        boxes[count] = (RenderLabelBox){left, top, text.x, text.y, priority, false};
        body_of[count++] = i;
    }
    render_declutter_labels(boxes, count);
    placed_label_count = 0;
    for (size_t k = 0; k < count; ++k) {
        if (!boxes[k].visible) continue;
        if (placed_label_count < RENDER_MAX_LABEL_BLOCKERS) {
            double *r = placed_labels[placed_label_count++];
            r[0] = boxes[k].x; r[1] = boxes[k].y; r[2] = boxes[k].width; r[3] = boxes[k].height;
        }
        const Body *body = &system->bodies[body_of[k]];
        Color color = body_of[k] == selected ? (Color){240, 200, 120, 255}
            : (int)body_of[k] == hovered ? (Color){255, 250, 240, 255} : (Color){228, 222, 206, 225};
        /* A soft drop shadow keeps pale text readable over bright planets. */
        DrawTextEx(font, body->name, (Vector2){boxes[k].x + s, boxes[k].y + s}, size, spacing, (Color){0, 0, 0, 170});
        DrawTextEx(font, body->name, (Vector2){boxes[k].x, boxes[k].y}, size, spacing, color);
        add_click_target(boxes[k].x, boxes[k].y, boxes[k].width, boxes[k].height, body_of[k]);
    }
}

/* Scale bar (SPEC A123), bottom left of the view: a round distance measured
 * at the depth of the point the camera orbits (the selected body or family),
 * so it reads true there; nearer or farther things scale with perspective.
 * In illustrative scale distances are true but bodies are enlarged, so the
 * bar measures space, not sizes. Its box is an obstacle for edge markers. */
static void draw_scale_bar(Font font, Camera3D camera)
{
    page_blocker_count = page_blocker_count > RENDER_MAX_BLOCKERS ? RENDER_MAX_BLOCKERS : page_blocker_count;
    blocker_count = page_blocker_count;
    const float s = pixel_scale;
    double depth = sqrt(pow(camera.target.x - camera.position.x, 2) + pow(camera.target.y - camera.position.y, 2)
        + pow(camera.target.z - camera.position.z, 2));
    double units_per_pixel = render_world_units_per_pixel(depth, camera.fovy, css_viewport_height());
    double units_per_meter = meters_to_render_units(1.0e9) / 1.0e9;
    RenderScaleBar bar;
    if (!(units_per_meter > 0) || !render_scale_bar(units_per_pixel / units_per_meter, 110.0, &bar)) return;
    const float size = 12.0f * s, spacing = 0.4f * s;
    Vector2 text = MeasureTextEx(font, bar.label, size, spacing);
    float length = (float)bar.pixels * s;
    float x = 16.0f * s, baseline = (float)GetScreenHeight() - 18.0f * s;
    const Color ink = {228, 222, 206, 200};
    DrawLineEx((Vector2){x, baseline}, (Vector2){x + length, baseline}, 1.5f * s, ink);
    DrawLineEx((Vector2){x, baseline - 5 * s}, (Vector2){x, baseline + 1 * s}, 1.5f * s, ink);
    DrawLineEx((Vector2){x + length, baseline - 5 * s}, (Vector2){x + length, baseline + 1 * s}, 1.5f * s, ink);
    DrawTextEx(font, bar.label, (Vector2){x, baseline - 8 * s - text.y}, size, spacing, ink);
    float width = fmaxf(length, text.x);
    blockers[blocker_count][0] = x - 6 * s;
    blockers[blocker_count][1] = baseline - 12 * s - text.y;
    blockers[blocker_count][2] = width + 12 * s;
    blockers[blocker_count][3] = text.y + 18 * s;
    ++blocker_count;
}

/* Off-screen markers (A119): the Sun, planets, dwarf planets and the
 * selection's parent, when outside the view (or behind the camera), get a
 * chevron and their name at the edge, in the direction to turn toward them.
 * Clicking one flies there. Decluttered like labels; presentation only. */
static void draw_offscreen_markers(const SolarSystem *system, RenderScaleMode mode, Vec3d origin, Font font,
    Camera3D camera, size_t selected)
{
    static RenderLabelBox boxes[SOLAR_SYSTEM_BODY_CAPACITY];
    static size_t body_of[SOLAR_SYSTEM_BODY_CAPACITY];
    static Vector2 tips[SOLAR_SYSTEM_BODY_CAPACITY], directions[SOLAR_SYSTEM_BODY_CAPACITY];
    const float s = pixel_scale, size = 13.0f * s, spacing = 0.4f * s;
    float width = (float)GetScreenWidth(), height = (float)GetScreenHeight();
    Vector3 forward = unit_vector((Vector3){camera.target.x - camera.position.x, camera.target.y - camera.position.y,
        camera.target.z - camera.position.z}, (Vector3){0, 0, -1});
    Vector3 right = unit_vector((Vector3){forward.y * camera.up.z - forward.z * camera.up.y,
        forward.z * camera.up.x - forward.x * camera.up.z, forward.x * camera.up.y - forward.y * camera.up.x}, (Vector3){1, 0, 0});
    Vector3 up = {right.y * forward.z - right.z * forward.y, right.z * forward.x - right.x * forward.z,
        right.x * forward.y - right.y * forward.x};
    int selected_parent = selected < system->body_count ? solar_system_parent_index(system, selected) : -1;
    /* Bodies large enough on screen to hide another (usually one to three),
     * found once per frame: testing all 300 bodies for every planet cost
     * the Saturn family 3 fps on the throttled phone. */
    static size_t occluder_index[SOLAR_SYSTEM_BODY_CAPACITY];
    static double occluder_distance[SOLAR_SYSTEM_BODY_CAPACITY], occluder_radius[SOLAR_SYSTEM_BODY_CAPACITY];
    static Vector2 occluder_screen[SOLAR_SYSTEM_BODY_CAPACITY];
    size_t occluders = 0;
    for (size_t j = 0; j < system->body_count && j < SOLAR_SYSTEM_BODY_CAPACITY; ++j) {
        if (system->bodies[j].radius_quality == PHYSICAL_UNKNOWN || (int)j == observer_body) continue;
        Vector3 pj = renderer_relative_vector(renderer_body_position(system, j, mode), origin);
        Vector3 oj = {pj.x - camera.position.x, pj.y - camera.position.y, pj.z - camera.position.z};
        if (oj.x * forward.x + oj.y * forward.y + oj.z * forward.z <= 0) continue;
        double dj = sqrt(oj.x * oj.x + oj.y * oj.y + oj.z * oj.z);
        /* Only Saturn's rings are opaque enough to hide a planet; the faint
         * ring systems are nearly transparent (review fix). */
        double hide = system->bodies[j].id == BODY_ID_SATURN ? renderer_body_visual_radius(&system->bodies[j], mode)
            : renderer_body_radius(&system->bodies[j], mode);
        double rj = render_projected_radius_pixels(hide, dj, camera.fovy, (double)height);
        if (rj <= 8.0 * s) continue;
        occluder_index[occluders] = j;
        occluder_distance[occluders] = dj;
        occluder_radius[occluders] = rj;
        occluder_screen[occluders++] = GetWorldToScreen(pj, camera);
    }
    size_t count = 0;
    for (size_t i = 0; i < system->body_count && i < SOLAR_SYSTEM_BODY_CAPACITY; ++i) {
        const Body *body = &system->bodies[i];
        bool major = body->kind == BODY_KIND_STAR || body->kind == BODY_KIND_PLANET || body->kind == BODY_KIND_DWARF_PLANET;
        if (i == selected || (int)i == observer_body || !(major || (int)i == selected_parent)) continue;
        Vector3 position = renderer_relative_vector(renderer_body_position(system, i, mode), origin);
        Vector3 offset = {position.x - camera.position.x, position.y - camera.position.y, position.z - camera.position.z};
        bool in_front = offset.x * forward.x + offset.y * forward.y + offset.z * forward.z > 0;
        double ax, ay;
        Vector2 unit, tip;
        bool behind_body = false;
        if (in_front) {
            Vector2 at = GetWorldToScreen(position, camera);
            if (at.x >= 0 && at.x <= width && at.y >= 0 && at.y <= height) {
                /* On screen: it has its own marks, unless a nearer body large
                 * on screen hides it (marked at that body's edge, on the side
                 * where it lies: "behind here") or a page panel covers it. */
                double distance = sqrt(offset.x * offset.x + offset.y * offset.y + offset.z * offset.z);
                int occluder = -1;
                double occluder_px = 0;
                Vector2 occluder_at = {0, 0};
                for (size_t k = 0; k < occluders; ++k) {
                    if (occluder_index[k] == i || occluder_distance[k] >= distance) continue;
                    Vector2 sj = occluder_screen[k];
                    if (occluder_radius[k] > occluder_px && hypot(at.x - sj.x, at.y - sj.y) < occluder_radius[k]) {
                        occluder = (int)occluder_index[k]; occluder_px = occluder_radius[k]; occluder_at = sj;
                    }
                }
                int panel = -1;
                /* Page panels and the scale bar only: a label is not a panel. */
                for (int b = 0; b < panel_blocker_count && panel < 0; ++b)
                    if (at.x >= blockers[b][0] && at.x <= blockers[b][0] + blockers[b][2] && at.y >= blockers[b][1]
                        && at.y <= blockers[b][1] + blockers[b][3]) panel = b;
                if (occluder >= 0) {
                    double ux = at.x - occluder_at.x, uy = at.y - occluder_at.y, ul = hypot(ux, uy);
                    unit = ul > 1e-6 ? (Vector2){(float)(ux / ul), (float)(uy / ul)} : (Vector2){0, -1};
                    tip = (Vector2){occluder_at.x + unit.x * (float)(occluder_px + 16.0 * s),
                        occluder_at.y + unit.y * (float)(occluder_px + 16.0 * s)};
                } else if (panel >= 0) {
                    /* Under a page panel: marked just outside the panel edge
                     * nearest to it that faces into the view, pointing in. */
                    const double *r = blockers[panel];
                    double edges[4] = {r[0] > 1 ? at.x - r[0] : 1e30, r[0] + r[2] < width - 1 ? r[0] + r[2] - at.x : 1e30,
                        r[1] > 1 ? at.y - r[1] : 1e30, r[1] + r[3] < height - 1 ? r[1] + r[3] - at.y : 1e30};
                    int e = 0;
                    for (int k = 1; k < 4; ++k) if (edges[k] < edges[e]) e = k;
                    if (edges[e] >= 1e29) continue;
                    const float gap = 8.0f * s;
                    if (e == 0) { tip = (Vector2){(float)r[0] - gap, at.y}; unit = (Vector2){1, 0}; }
                    else if (e == 1) { tip = (Vector2){(float)(r[0] + r[2]) + gap, at.y}; unit = (Vector2){-1, 0}; }
                    else if (e == 2) { tip = (Vector2){at.x, (float)r[1] - gap}; unit = (Vector2){0, 1}; }
                    else { tip = (Vector2){at.x, (float)(r[1] + r[3]) + gap}; unit = (Vector2){0, -1}; }
                } else {
                    continue; /* visible: its own dot, ring and label serve */
                }
                ax = tip.x - width / 2.0; ay = tip.y - height / 2.0;
                behind_body = true;
            }
        }
        if (!behind_body) {
            double dx = offset.x * right.x + offset.y * right.y + offset.z * right.z;
            double dy = -(offset.x * up.x + offset.y * up.y + offset.z * up.z);
            render_edge_anchor(dx, dy, width / 2.0, height / 2.0, 18.0 * s, &ax, &ay);
            double length = hypot(ax, ay);
            unit = length > 0 ? (Vector2){(float)(ax / length), (float)(ay / length)} : (Vector2){0, 1};
            tip = (Vector2){width / 2.0f + (float)ax, height / 2.0f + (float)ay};
        }
        Vector2 text = MeasureTextEx(font, body->name, size, spacing);
        /* The name sits inward of the chevron, toward the view's centre. */
        float cx = tip.x - unit.x * (12.0f * s + text.x / 2), cy = tip.y - unit.y * (10.0f * s + text.y / 2);
        int priority = body->kind == BODY_KIND_STAR ? 900 : (int)i == selected_parent ? 850
            : body->kind == BODY_KIND_PLANET ? 800 - (int)i : 600 - (int)i;
        boxes[count] = (RenderLabelBox){cx - text.x / 2 - 4 * s, cy - text.y / 2 - 2 * s, text.x + 8 * s, text.y + 4 * s, priority, false};
        /* Slide along the edge past page panels floating over the canvas. */
        bool side = fabs(ax) / fmax(width / 2.0 - 18.0 * s, 1) >= fabs(ay) / fmax(height / 2.0 - 18.0 * s, 1);
        for (int pass = 0; pass < 2; ++pass) {
            for (int k = 0; k < blocker_count; ++k) {
                double shift = render_slide_past(boxes[count].x, boxes[count].y, boxes[count].width, boxes[count].height,
                    blockers[k], side, 8.0 * s, side ? height - 8.0 * s : width - 8.0 * s);
                if (side) { boxes[count].y += (float)shift; tip.y += (float)shift; }
                else { boxes[count].x += (float)shift; tip.x += (float)shift; }
            }
        }
        tips[count] = tip;
        directions[count] = unit;
        body_of[count++] = i;
    }
    /* Place in priority order. A marker that would overlap one already placed
     * (or a page panel) steps along its edge, alternating sides, until it
     * fits: every off-screen planet stays reachable instead of being hidden. */
    static size_t order[SOLAR_SYSTEM_BODY_CAPACITY];
    static bool sides[SOLAR_SYSTEM_BODY_CAPACITY];
    for (size_t k = 0; k < count; ++k) order[k] = k;
    for (size_t a = 1; a < count; ++a)
        for (size_t b = a; b > 0 && boxes[order[b]].priority > boxes[order[b - 1]].priority; --b) {
            size_t t = order[b]; order[b] = order[b - 1]; order[b - 1] = t;
        }
    for (size_t k = 0; k < count; ++k) {
        float tx = tips[k].x - width / 2.0f, ty = tips[k].y - height / 2.0f;
        sides[k] = fabsf(tx) / fmaxf(width / 2.0f - 18.0f * s, 1) >= fabsf(ty) / fmaxf(height / 2.0f - 18.0f * s, 1);
    }
    for (size_t n = 0; n < count; ++n) {
        size_t k = order[n];
        RenderLabelBox start = boxes[k];
        float step = (sides[k] ? start.height : start.width) + 4.0f * s;
        boxes[k].visible = false;
        /* Enough steps to walk the whole edge in both directions. */
        int attempts = 2 * (int)ceilf((sides[k] ? height : width) / step) + 1;
        for (int attempt = 0; attempt < attempts && !boxes[k].visible; ++attempt) {
            float offset = step * (float)((attempt + 1) / 2) * (attempt % 2 ? 1.0f : -1.0f);
            RenderLabelBox trial = start;
            if (sides[k]) trial.y += offset; else trial.x += offset;
            if (trial.x < 4 * s || trial.y < 4 * s || trial.x + trial.width > width - 4 * s || trial.y + trial.height > height - 4 * s)
                continue;
            bool clear = true;
            for (size_t m = 0; m < n && clear; ++m) {
                const RenderLabelBox *o = &boxes[order[m]];
                clear = !o->visible || trial.x >= o->x + o->width || trial.x + trial.width <= o->x
                    || trial.y >= o->y + o->height || trial.y + trial.height <= o->y;
            }
            for (int b = 0; b < blocker_count && clear; ++b)
                clear = trial.x >= blockers[b][0] + blockers[b][2] || trial.x + trial.width <= blockers[b][0]
                    || trial.y >= blockers[b][1] + blockers[b][3] || trial.y + trial.height <= blockers[b][1];
            if (clear) {
                if (sides[k]) tips[k].y += offset; else tips[k].x += offset;
                boxes[k] = trial;
                boxes[k].visible = true;
            }
        }
    }
    for (size_t k = 0; k < count; ++k) {
        if (!boxes[k].visible) continue;
        const Body *body = &system->bodies[body_of[k]];
        Color color = renderer_body_color(body);
        Color ink = {(unsigned char)((color.r + 2 * 228) / 3), (unsigned char)((color.g + 2 * 222) / 3),
            (unsigned char)((color.b + 2 * 206) / 3), 230};
        /* A chevron pointing out of the view, toward the body. */
        Vector2 u = directions[k], n = {-u.y, u.x}, tip = tips[k];
        Vector2 back = {tip.x - u.x * 8.0f * s, tip.y - u.y * 8.0f * s};
        DrawLineEx((Vector2){back.x + n.x * 5.0f * s, back.y + n.y * 5.0f * s}, tip, 1.5f * s, ink);
        DrawLineEx((Vector2){back.x - n.x * 5.0f * s, back.y - n.y * 5.0f * s}, tip, 1.5f * s, ink);
        DrawRectangleRec((Rectangle){boxes[k].x, boxes[k].y, boxes[k].width, boxes[k].height}, (Color){10, 12, 18, 150});
        DrawTextEx(font, body->name, (Vector2){boxes[k].x + 4 * s, boxes[k].y + 2 * s}, size, spacing, ink);
        add_click_target(boxes[k].x, boxes[k].y, boxes[k].width, boxes[k].height, body_of[k]);
        /* The chevron itself is clickable too. */
        add_click_target(tip.x - 10.0f * s, tip.y - 10.0f * s, 20.0f * s, 20.0f * s, body_of[k]);
    }
}

void renderer_draw_markers(const SolarSystem *system, RenderScaleMode mode, Vec3d origin,
    const RenderResources *resources, Camera3D camera, size_t selected, int hovered)
{
    click_target_count = 0;
    Font font = resources->label_font_ready ? resources->label_font : GetFontDefault();
    draw_scale_bar(font, camera);
    /* Edge markers step past the names in view (labels draw after markers,
     * so last frame's are used): through the telescope a "Sun >" chevron
     * must not land on Ganymede. */
    /* The app's canvas caption (the view from Earth's) is an obstacle too. */
    if (caption_box[2] > 0 && blocker_count < RENDER_MAX_BLOCKERS + 1) {
        for (int c = 0; c < 4; ++c) blockers[blocker_count][c] = caption_box[c];
        ++blocker_count;
    }
    panel_blocker_count = blocker_count;
    for (int k = 0; k < placed_label_count; ++k)
        for (int c = 0; c < 4; ++c) blockers[blocker_count + k][c] = placed_labels[k][c];
    blocker_count += placed_label_count;
    placed_label_count = 0; /* refilled if labels draw this frame */
    draw_offscreen_markers(system, mode, origin, font, camera, selected);
    Vector3 forward = unit_vector((Vector3){camera.target.x - camera.position.x, camera.target.y - camera.position.y,
        camera.target.z - camera.position.z}, (Vector3){0, 0, -1});
    double viewport_height = css_viewport_height();
    const float s = pixel_scale;
    for (size_t i = 0; i < system->body_count && i < SOLAR_SYSTEM_BODY_CAPACITY; ++i) {
        const Body *body = &system->bodies[i];
        bool is_selected = i == selected, is_hovered = (int)i == hovered;
        if ((int)i == observer_body) continue;
        if (!is_selected && !is_hovered && body->kind != BODY_KIND_PLANET && body->kind != BODY_KIND_DWARF_PLANET) continue;
        Vector3 position = renderer_relative_vector(renderer_body_position(system, i, mode), origin);
        Vector3 offset = {position.x - camera.position.x, position.y - camera.position.y, position.z - camera.position.z};
        if (offset.x * forward.x + offset.y * forward.y + offset.z * forward.z <= 0) continue;
        double distance = sqrt(offset.x * offset.x + offset.y * offset.y + offset.z * offset.z);
        double radius_px = body->radius_quality == PHYSICAL_UNKNOWN ? 0
            : render_projected_radius_pixels(renderer_body_radius(body, mode), distance, camera.fovy, viewport_height);
        Vector2 at = GetWorldToScreen(position, camera);
        if (is_hovered && !is_selected) {
            /* The body under the pointer: a brighter ring just outside it. */
            float ring = (float)render_hover_ring_radius(radius_px) * s;
            DrawRing(at, ring - 0.75f * s, ring + 0.75f * s, 0, 360, 48, (Color){255, 250, 240, 210});
        } else if (render_body_wants_ring_marker(body->kind, radius_px) && !is_selected) {
            /* A thin ring in the body's colour: findable, but quieter than
             * the dot it surrounds. */
            Color ring = renderer_body_color(body);
            ring.a = 120;
            DrawRing(at, (RENDER_RING_MARKER_RADIUS_PIXELS - 0.6f) * s, (RENDER_RING_MARKER_RADIUS_PIXELS + 0.6f) * s, 0, 360, 40, ring);
        }
        if (is_selected && render_selection_wants_brackets(radius_px)) {
            /* Four amber corner brackets, the instrument's one accent colour
             * (DESIGN.md), outside the drawn disc at every zoom. */
            float half = (float)render_selection_bracket_half_size(radius_px) * s;
            float arm = fminf(9.0f * s, half * 0.45f);
            const Color amber = {240, 200, 120, 235};
            for (int corner = 0; corner < 4; ++corner) {
                float sx = corner & 1 ? 1.0f : -1.0f, sy = corner & 2 ? 1.0f : -1.0f;
                Vector2 tip = {at.x + sx * half, at.y + sy * half};
                DrawLineEx(tip, (Vector2){tip.x - sx * arm, tip.y}, 1.5f * s, amber);
                DrawLineEx(tip, (Vector2){tip.x, tip.y - sy * arm}, 1.5f * s, amber);
            }
        }
    }
}

int renderer_pick_body(const SolarSystem *system, RenderScaleMode mode, Vec3d origin, Camera3D camera, Vector2 pointer)
{
    static RenderPickCandidate candidates[SOLAR_SYSTEM_BODY_CAPACITY];
    Vector3 forward = unit_vector((Vector3){camera.target.x - camera.position.x, camera.target.y - camera.position.y,
        camera.target.z - camera.position.z}, (Vector3){0, 0, -1});
    double viewport_height = css_viewport_height();
    size_t count = system->body_count < SOLAR_SYSTEM_BODY_CAPACITY ? system->body_count : SOLAR_SYSTEM_BODY_CAPACITY;
    for (size_t i = 0; i < count; ++i) {
        const Body *body = &system->bodies[i];
        /* The static slot must be overwritten, not skipped: a stale entry
         * from an earlier frame would make the observer pickable (Bugbot). */
        if ((int)i == observer_body) { candidates[i] = (RenderPickCandidate){.kind = body->kind, .in_front = false}; continue; }
        Vector3 position = renderer_relative_vector(renderer_body_position(system, i, mode), origin);
        Vector3 offset = {position.x - camera.position.x, position.y - camera.position.y, position.z - camera.position.z};
        bool in_front = offset.x * forward.x + offset.y * forward.y + offset.z * forward.z > 0;
        Vector2 at = in_front ? GetWorldToScreen(position, camera) : (Vector2){0, 0};
        double distance = sqrt(offset.x * offset.x + offset.y * offset.y + offset.z * offset.z);
        float radius = body->radius_quality == PHYSICAL_UNKNOWN ? 0
            : (float)render_projected_radius_pixels(renderer_body_radius(body, mode), distance, camera.fovy, viewport_height);
        /* Picking works in CSS pixels, like its slop. */
        candidates[i] = (RenderPickCandidate){at.x / pixel_scale, at.y / pixel_scale, radius, body->kind, in_front};
    }
    return render_pick_body(candidates, count, pointer.x / pixel_scale, pointer.y / pixel_scale);
}

/* Everything the drawing phases share in one frame, computed once: camera
 * geometry, the light source, and each body's camera-relative position,
 * drawn radius and distance. */
typedef struct FrameContext {
    const SolarSystem *system;
    const BodyTrails *trails;
    const RenderResources *resources;
    const RenderView *view;
    RenderScaleMode mode;
    RenderTrailFrame trail_frame;
    Vec3d origin;
    Camera3D camera;
    Vector3 forward;
    double camera_distance, viewport_height;
    int star; /* index of the light source, or -1 (lit from the camera) */
    Vector3 star_position;
    RenderFrameCache cache;
    Vector3 position[SOLAR_SYSTEM_BODY_CAPACITY];
    float radius[SOLAR_SYSTEM_BODY_CAPACITY];
    double distance[SOLAR_SYSTEM_BODY_CAPACITY];
} FrameContext;

static void frame_context_build(FrameContext *ctx)
{
    Camera3D camera = ctx->view->camera;
    ctx->camera = camera;
    Vector3 to_camera = {camera.position.x - camera.target.x, camera.position.y - camera.target.y, camera.position.z - camera.target.z};
    ctx->camera_distance = sqrt(to_camera.x * to_camera.x + to_camera.y * to_camera.y + to_camera.z * to_camera.z);
    ctx->forward = unit_vector((Vector3){-to_camera.x, -to_camera.y, -to_camera.z}, (Vector3){0, 0, -1});
    ctx->viewport_height = css_viewport_height();
    renderer_frame_cache_build(&ctx->cache, ctx->system, ctx->mode);
    ctx->star = -1;
    for (size_t i = 0; i < ctx->system->body_count && i < SOLAR_SYSTEM_BODY_CAPACITY; ++i) {
        ctx->position[i] = renderer_relative_vector(ctx->cache.position[i], ctx->origin);
        ctx->radius[i] = renderer_body_radius(&ctx->system->bodies[i], ctx->mode);
        Vector3 d = {ctx->position[i].x - camera.position.x, ctx->position[i].y - camera.position.y, ctx->position[i].z - camera.position.z};
        ctx->distance[i] = sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
        if (ctx->star < 0 && ctx->system->bodies[i].kind == BODY_KIND_STAR) ctx->star = (int)i;
    }
    /* Lessons without a star (the two-sphere contact demo) are lit from the camera. */
    ctx->star_position = ctx->star >= 0 ? ctx->position[ctx->star] : camera.position;
}

/* Unit vector from body i toward the light source. */
static Vector3 light_toward_star(const FrameContext *ctx, size_t i)
{
    Vector3 p = ctx->position[i];
    return unit_vector((Vector3){ctx->star_position.x - p.x, ctx->star_position.y - p.y, ctx->star_position.z - p.z},
        (Vector3){0, 1, 0});
}

static double body_pixels(const FrameContext *ctx, size_t i, float radius)
{
    return render_projected_radius_pixels(radius, ctx->distance[i], ctx->camera.fovy, ctx->viewport_height);
}

/* Orbit guides (SPEC A107): each body's osculating two-body ellipse about
 * its parent, from this frame's SI state. Presentation only; nothing here is
 * stored or fed back. The polyline starts at the body's own eccentric anomaly
 * so a vertex sits exactly on the body, and E advances by a fixed rotation
 * (two multiplies per vertex instead of cos/sin). When the camera is close to
 * the orbit, an even step would leave long chords that visibly miss the body,
 * so the spacing is cubic in the parameter: dense at the body, coarse on the
 * far side, which is also farthest from the camera. */
static void draw_orbit_guides(const FrameContext *ctx)
{
    const SolarSystem *system = ctx->system;
    Vec3d camera_world = vec3d_add(ctx->origin, (Vec3d){ctx->camera.position.x, ctx->camera.position.y, ctx->camera.position.z});
    RenderFrustum view = render_frustum(camera_world,
        (Vec3d){ctx->forward.x, ctx->forward.y, ctx->forward.z},
        (Vec3d){ctx->camera.up.x, ctx->camera.up.y, ctx->camera.up.z}, ctx->camera.fovy,
        render_aspect(), 0.05);
    const double pi = 3.14159265358979323846;
    rlBegin(RL_LINES);
    for (size_t i = 0; i < system->body_count && i < SOLAR_SYSTEM_BODY_CAPACITY; ++i) {
        const Body *body = &system->bodies[i];
        int parent_index = ctx->cache.parent[i];
        if (body->kind == BODY_KIND_STAR || parent_index < 0 || (int)i == observer_body) continue;
        const Body *parent = &system->bodies[parent_index];
        Vec3d r = vec3d_sub(body->position_m, parent->position_m);
        Vec3d v = vec3d_sub(body->velocity_mps, parent->velocity_mps);
        RenderOrbitGuide guide;
        if (!render_orbit_guide(r, v, SOLAR_G * (parent->mass_kg + body->mass_kg), &guide)) continue;

        /* Screen size: the semi-major axis at the parent's distance, scaled
         * by how far the illustrative view spreads this body from its parent. */
        Vec3d parent_render = ctx->cache.position[parent_index];
        double true_distance = meters_to_render_units(vec3d_length(r));
        double drawn_distance = vec3d_length(vec3d_sub(ctx->cache.position[i], parent_render));
        double spread_factor = true_distance > 0 ? drawn_distance / true_distance : 1.0;
        double a_units = meters_to_render_units(guide.semi_major_m) * spread_factor;
        double parent_distance = fmax(vec3d_length(vec3d_sub(parent_render, camera_world)), 1e-12);
        double a_pixels = render_projected_radius_pixels(a_units, parent_distance, ctx->camera.fovy, ctx->viewport_height);
        /* Faint guides (moons of unknown size, hundreds in the Saturn and
         * Jupiter families) are drawn at half detail: at 7% opacity a
         * coarser chord is invisible, and they dominate the vertex count. */
        bool faint = body->radius_quality == PHYSICAL_UNKNOWN && i != ctx->view->selected;
        int segments = a_pixels >= RENDER_ORBIT_GUIDE_MIN_PIXELS ? render_orbit_guide_segments(faint
            ? fmax(a_pixels * 0.5, RENDER_ORBIT_GUIDE_MIN_PIXELS) : a_pixels) : 0;
        if (!segments) continue;

        /* Planets read first, then dwarf planets and moons; markers of
         * unknown size (mostly tiny irregular moons) stay faint, so a 291-moon
         * family is a haze of paths, not a scribble. Small guides fade in. */
        float alpha = body->kind == BODY_KIND_PLANET ? 0.34f : body->kind == BODY_KIND_DWARF_PLANET ? 0.28f
            : body->radius_quality == PHYSICAL_UNKNOWN ? 0.07f : 0.22f;
        if (i == ctx->view->selected) alpha = 0.5f;
        alpha *= (float)fmin(1.0, (a_pixels - RENDER_ORBIT_GUIDE_MIN_PIXELS) / 24.0 + 0.15);
        Color color = renderer_body_color(body);

        bool spread = ctx->mode != RENDER_SCALE_REAL && !(body->kind != BODY_KIND_MOON && parent->kind == BODY_KIND_STAR);
        /* Illustrative spreading pushes every moon out by its clearance
         * (A135), so every guide follows it. The clearance depends on the
         * distance only where an orbit crosses a ring edge; otherwise it is
         * one number for the whole orbit. The Moon's eightfold expansion
         * keeps the full per-vertex form. */
        double clearance = -1.0;
        if (spread && body->id != BODY_ID_MOON) {
            double periapsis = meters_to_render_units(guide.semi_major_m * (1 - guide.eccentricity));
            double apoapsis = meters_to_render_units(guide.semi_major_m * (1 + guide.eccentricity));
            double at_periapsis = satellite_clearance(body, parent, periapsis, ctx->mode);
            /* Equal unless the orbit crosses a ring edge, where the two
             * differ by a whole ring extent; the tolerance only absorbs
             * rounding. */
            if (fabs(at_periapsis - satellite_clearance(body, parent, apoapsis, ctx->mode)) <= 1e-9 * at_periapsis)
                clearance = at_periapsis;
        }
        /* The body's eccentric anomaly from its perifocal coordinates:
         * x = a (cos E - e), y = b sin E. */
        double cos0 = vec3d_dot(r, guide.periapsis) / guide.semi_major_m + guide.eccentricity;
        double sin0 = vec3d_dot(r, guide.ahead) / guide.semi_minor_m;
        double norm = hypot(cos0, sin0);
        cos0 = norm > 0 ? cos0 / norm : 1.0;
        sin0 = norm > 0 ? sin0 / norm : 0.0;
        double e0 = atan2(sin0, cos0);
        bool close = a_pixels > 2500.0;
        double step_cos = cos(2 * pi / segments), step_sin = sin(2 * pi / segments);
        double c = cos0, s = sin0;
        Vec3d previous = orbit_guide_vertex(system, parent_index, body, parent, parent_render, render_orbit_guide_point(&guide, c, s), ctx->mode, spread, clearance);
        unsigned previous_code = render_frustum_outcode(&view, previous);
        for (int k = 1; k <= segments; ++k) {
            if (close) {
                /* u in [-1, 1] around the orbit; E - E0 = pi u^3 is dense at the
                 * body (u = 0, the two ends meet on the far side). */
                double u = -1.0 + 2.0 * k / segments;
                double e = e0 + pi * u * u * u;
                c = cos(e); s = sin(e);
                if (k == 1) {
                    double e_start = e0 - pi;
                    previous = orbit_guide_vertex(system, parent_index, body, parent, parent_render,
                        render_orbit_guide_point(&guide, cos(e_start), sin(e_start)), ctx->mode, spread, clearance);
                    previous_code = render_frustum_outcode(&view, previous);
                }
            } else {
                double next_c = c * step_cos - s * step_sin;
                s = s * step_cos + c * step_sin;
                c = next_c;
            }
            Vec3d point = orbit_guide_vertex(system, parent_index, body, parent, parent_render, render_orbit_guide_point(&guide, c, s), ctx->mode, spread, clearance);
            unsigned code = render_frustum_outcode(&view, point);
            if (!(code & previous_code)) {
                line_vertex(renderer_relative_vector(previous, ctx->origin), color, alpha);
                line_vertex(renderer_relative_vector(point, ctx->origin), color, alpha);
            }
            previous = point;
            previous_code = code;
        }
    }
    rlEnd();
}

/* Small-body cloud (SPEC A121): the GPU moves every sampled body along its
 * heliocentric conic for today's date (see the vertex shader); here only the
 * Sun's place, the scale and the time are set. Additive, depth-tested, no
 * depth writes: planets hide what lies behind them. Presentation only. */
static void draw_belt(const FrameContext *ctx)
{
    const RenderResources *resources = ctx->resources;
    if (ctx->star < 0 || !resources->belt_mesh_count) return;
    float days = (float)ctx->view->days_since_epoch;
    float sun[3] = {ctx->star_position.x, ctx->star_position.y, ctx->star_position.z};
    float au = meters_to_render_units(SOLAR_AU_METERS);
    float viewport[2] = {(float)GetRenderWidth(), (float)GetRenderHeight()};
    float size = 1.4f * pixel_scale;
    SetShaderValue(resources->belt_shader, resources->loc_belt_days, &days, SHADER_UNIFORM_FLOAT);
    SetShaderValue(resources->belt_shader, resources->loc_belt_sun, sun, SHADER_UNIFORM_VEC3);
    SetShaderValue(resources->belt_shader, resources->loc_belt_au, &au, SHADER_UNIFORM_FLOAT);
    SetShaderValue(resources->belt_shader, resources->loc_belt_viewport, viewport, SHADER_UNIFORM_VEC2);
    SetShaderValue(resources->belt_shader, resources->loc_belt_size, &size, SHADER_UNIFORM_FLOAT);
    /* In CSS pixels, like every other budget, so HiDPI does not change it. */
    float focal = (float)(ctx->viewport_height / (2.0 * tan(ctx->camera.fovy * DEG2RAD / 2.0)));
    SetShaderValue(resources->belt_shader, resources->loc_belt_focal, &focal, SHADER_UNIFORM_FLOAT);
    rlDrawRenderBatchActive();
    for (int k = 0; k < resources->belt_mesh_count; ++k)
        DrawMesh(resources->belt_meshes[k], resources->belt_material, (Matrix){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});
}

/* Phase 1. Milky Way backdrop: a huge inside-out sphere centred on the camera,
 * drawn first without writing depth, so everything else draws over it and it
 * never moves as the camera translates (stars are effectively at infinity).
 * It is a real all-sky map, oriented into simulation axes (SPEC A117). */
/* Constellation lines (SPEC A124) on a sphere just inside the sky's, centred
 * on the camera like it, so they sit on the map's stars at infinity. */
static void draw_constellations(const FrameContext *ctx)
{
    const float radius = ctx->view->far_plane * 0.45f;
    const Vector3 c = ctx->camera.position;
    rlBegin(RL_LINES);
    rlColor4ub(120, 150, 205, 70);
    for (size_t k = 0; k < render_constellation_count(); ++k) {
        Vec3d a, b;
        render_constellation_segment(k, &a, &b);
        rlVertex3f(c.x + radius * (float)a.x, c.y + radius * (float)a.y, c.z + radius * (float)a.z);
        rlVertex3f(c.x + radius * (float)b.x, c.y + radius * (float)b.y, c.z + radius * (float)b.z);
    }
    rlEnd();
    rlDrawRenderBatchActive();
}

static void draw_backdrop(const FrameContext *ctx)
{
    const RenderResources *resources = ctx->resources;
    if (!resources->ready || !resources->texture_loaded[RENDER_TEXTURE_STARS]) {
        if (ctx->view->show_constellations) {
            rlDisableDepthMask();
            draw_constellations(ctx);
            rlEnableDepthMask();
        }
        return;
    }
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    rlDisableBackfaceCulling();
    /* The map is the real sky in galactic coordinates; oriented, its stars
     * and the Milky Way sit in their true directions (A117). */
    Matrix sky = body_matrix(render_sky_orientation(), ctx->camera.position, ctx->view->far_plane * 0.5f);
    draw_shaded(resources, &resources->sphere_detailed, &sky, SHADE_SKY, resources->textures[RENDER_TEXTURE_STARS],
        WHITE, (Vector3){0, 1, 0}, (RenderAtmosphere){0}, NULL, NULL);
    if (ctx->view->show_constellations) draw_constellations(ctx);
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}

/* Phase 2. Reference grid: it writes no depth, so every body drawn later
 * covers it even where the plane passes through the body's centre; the grid
 * reads as a background reference rather than slicing planets in half. */
static void draw_grid(const FrameContext *ctx)
{
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    if (ctx->view->show_grid)
        draw_reference_grid(ctx->origin, ctx->camera_distance, &ctx->camera, render_aspect());
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}

/* The unknown-radius wire marker (V25): the same 4-ring, 4-slice line sphere
 * as raylib's DrawSphereWires, whose unit vertices are computed once here.
 * DrawSphereWires evaluates sinf/cosf for every vertex and pushes a matrix
 * that rlVertex3f then applies to each one; with 275 unknown-radius moons in
 * the Saturn scene that was a tenth of the frame on a throttled phone. */
#define WIRE_MARKER_RINGS 4
#define WIRE_MARKER_SLICES 4
#define WIRE_MARKER_VERTICES ((WIRE_MARKER_RINGS + 2) * WIRE_MARKER_SLICES * 6)

static void draw_wire_marker(Vector3 center, float radius, Color color)
{
    static Vector3 unit[WIRE_MARKER_VERTICES];
    static bool ready;
    if (!ready) {
        size_t n = 0;
        for (int i = 0; i < WIRE_MARKER_RINGS + 2; ++i) {
            for (int j = 0; j < WIRE_MARKER_SLICES; ++j) {
                /* Ring latitude and slice longitude of each line end, in
                 * DrawSphereWires's order: (i, j)-(i+1, j+1), (i+1, j+1)-(i+1, j),
                 * (i+1, j)-(i, j). */
                const int ends[6][2] = {{i, j}, {i + 1, j + 1}, {i + 1, j + 1}, {i + 1, j}, {i + 1, j}, {i, j}};
                for (int k = 0; k < 6; ++k) {
                    float latitude = DEG2RAD * (270 + (180.0f / (WIRE_MARKER_RINGS + 1)) * ends[k][0]);
                    float longitude = DEG2RAD * (360.0f * ends[k][1] / WIRE_MARKER_SLICES);
                    unit[n++] = (Vector3){cosf(latitude) * sinf(longitude), sinf(latitude), cosf(latitude) * cosf(longitude)};
                }
            }
        }
        ready = true;
    }
    rlBegin(RL_LINES);
    rlColor4ub(color.r, color.g, color.b, color.a);
    for (size_t n = 0; n < WIRE_MARKER_VERTICES; ++n)
        rlVertex3f(center.x + radius * unit[n].x, center.y + radius * unit[n].y, center.z + radius * unit[n].z);
    rlEnd();
}

/* Eclipse casters for body i (SPEC A111): up to RENDER_MAX_SHADOW_CASTERS
 * bodies of its own family (its parent, its moons, its siblings) whose
 * spheres block part of the Sun as seen from body i in the true SI geometry.
 * Each is placed as a copy of that geometry scaled by body i's drawn/true
 * radius around its drawn centre, so an eclipse shows exactly when one really
 * happens, at its true size relative to the body, even when illustrative
 * scale enlarges bodies relative to their spacing. Presentation only. */
static void set_eclipse_casters(const FrameContext *ctx, size_t i)
{
    const RenderResources *resources = ctx->resources;
    float casters[RENDER_MAX_SHADOW_CASTERS * 4] = {0};
    float scores[RENDER_MAX_SHADOW_CASTERS];
    int count = 0;
    float sun_radius = 0;
    if (ctx->star >= 0 && resources->loc_occluders >= 0 && ctx->system->bodies[i].radius_m > 0) {
        const SolarSystem *system = ctx->system;
        const Body *body = &system->bodies[i];
        double sun_distance = vec3d_length(vec3d_sub(body->position_m, system->bodies[ctx->star].position_m));
        sun_radius = sun_distance > 0 ? (float)(system->bodies[ctx->star].radius_m / sun_distance) : 0;
        /* Drawn radius per true metre: 1 render unit per its metres in real
         * scale, larger for an enlarged illustrative body. */
        double scale = ctx->radius[i] / body->radius_m;
        int parent = ctx->cache.parent[i];
        for (size_t j = 0; j < system->body_count && j < SOLAR_SYSTEM_BODY_CAPACITY; ++j) {
            const Body *other = &system->bodies[j];
            if (j == i || other->kind == BODY_KIND_STAR || other->radius_quality == PHYSICAL_UNKNOWN) continue;
            int other_parent = ctx->cache.parent[j];
            bool family = other_parent == (int)i || parent == (int)j
                || (parent >= 0 && other_parent == parent && system->bodies[parent].kind != BODY_KIND_STAR);
            if (!family) continue;
            RenderEclipseCaster caster;
            if (!render_eclipse_caster(body->position_m, body->radius_m, other->position_m, other->radius_m,
                    system->bodies[ctx->star].position_m, system->bodies[ctx->star].radius_m, scale, &caster)) continue;
            float gap = (float)caster.gap;
            int slot = count < RENDER_MAX_SHADOW_CASTERS ? count++ : -1;
            if (slot < 0) {
                /* Keep the casters closest to the shadow line. */
                slot = 0;
                for (int k = 1; k < RENDER_MAX_SHADOW_CASTERS; ++k) if (scores[k] > scores[slot]) slot = k;
                if (gap >= scores[slot]) continue;
            }
            scores[slot] = gap;
            casters[slot * 4 + 0] = ctx->position[i].x + (float)caster.offset.x;
            casters[slot * 4 + 1] = ctx->position[i].y + (float)caster.offset.y;
            casters[slot * 4 + 2] = ctx->position[i].z + (float)caster.offset.z;
            casters[slot * 4 + 3] = (float)caster.radius;
        }
    }
    float count_value = (float)count;
    if (resources->loc_occluder_count >= 0)
        SetShaderValue(resources->shader, resources->loc_occluder_count, &count_value, SHADER_UNIFORM_FLOAT);
    if (resources->loc_sun_radius >= 0)
        SetShaderValue(resources->shader, resources->loc_sun_radius, &sun_radius, SHADER_UNIFORM_FLOAT);
    if (count && resources->loc_occluders >= 0)
        SetShaderValueV(resources->shader, resources->loc_occluders, casters, SHADER_UNIFORM_VEC4, count);
}

/* Phase 3. Opaque bodies. Each mesh draw costs dozens of WebGL calls, so
 * bodies behind the camera are skipped and sub-pixel bodies become one
 * batched dot (presentation only: their SI state is untouched). */
static void draw_opaque_bodies(const FrameContext *ctx)
{
    const SolarSystem *system = ctx->system;
    const RenderResources *resources = ctx->resources;
    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        if ((int)i == observer_body) continue;
        Vector3 position = ctx->position[i];
        float radius = ctx->radius[i];
        Vector3 offset = {position.x - ctx->camera.position.x, position.y - ctx->camera.position.y, position.z - ctx->camera.position.z};
        double depth = offset.x * ctx->forward.x + offset.y * ctx->forward.y + offset.z * ctx->forward.z;
        if (depth < -(double)renderer_body_visual_radius(body, ctx->mode)) continue;
        if (resources->ready && body->kind != BODY_KIND_STAR && body_pixels(ctx, i, radius) < RENDER_MESH_MIN_RADIUS_PIXELS) {
            /* A tiny camera-facing square a few pixels wide. (raylib's
             * DrawPoint3D draws a 0.1-unit line along +Z, which in real scale
             * spans thousands of pixels.) */
            float dot = (float)(RENDER_DOT_PIXELS * render_world_units_per_pixel(ctx->distance[i], ctx->camera.fovy, ctx->viewport_height));
            DrawBillboard(ctx->camera, resources->white, position, dot, renderer_body_color(body));
            continue;
        }
        if (body->radius_quality == PHYSICAL_UNKNOWN && resources->ready && !render_marker_draws_wire(body_pixels(ctx, i, radius))) {
            /* Too small to read as a wireframe: a dot of the same size. */
            float dot = (float)(render_marker_dot_pixels(body_pixels(ctx, i, radius))
                * render_world_units_per_pixel(ctx->distance[i], ctx->camera.fovy, ctx->viewport_height));
            DrawBillboard(ctx->camera, resources->white, position, dot, renderer_body_color(body));
            continue;
        }
        if (body->radius_quality == PHYSICAL_UNKNOWN || !resources->ready) {
            /* Unknown radius: an explicitly nonphysical wire marker (V25). */
            if (body->radius_quality == PHYSICAL_UNKNOWN) draw_wire_marker(position, radius, renderer_body_color(body));
            else DrawSphere(position, radius, renderer_body_color(body));
            continue;
        }
        RenderTextureSlot slot = render_texture_for_body(body->id);
        bool textured = slot >= 0 && resources->texture_loaded[slot];
        RenderOrientation orientation = render_body_orientation(body->id, ctx->view->orientation_days);
        Matrix transform = body_matrix(orientation, position, radius);
        const Mesh *mesh = textured || body->kind != BODY_KIND_MOON ? &resources->sphere_detailed : &resources->sphere_simple;
        Color tint = textured ? WHITE : renderer_body_color(body);
        if ((int)i == ctx->star) {
            draw_shaded(resources, mesh, &transform, SHADE_STAR, texture_or_white(resources, slot), tint,
                (Vector3){0, 1, 0}, (RenderAtmosphere){0}, NULL, NULL);
            continue;
        }
        bool city_lights = body->id == BODY_ID_EARTH && resources->texture_loaded[RENDER_TEXTURE_EARTH_NIGHT];
        /* The rings' shadow on Saturn or Uranus needs the ring opacity map. */
        RingShadow saturn_rings;
        bool ringed = (body->id == BODY_ID_SATURN || ring_system_index(body->id) >= 0)
            && planet_ring_shadow(resources, body->id, orientation, position, radius, ctx->mode, &saturn_rings);
        set_eclipse_casters(ctx, i);
        draw_shaded(resources, mesh, &transform, SHADE_LIT, texture_or_white(resources, slot), tint, light_toward_star(ctx, i),
            render_atmosphere_for_body(body->id), city_lights ? &resources->textures[RENDER_TEXTURE_EARTH_NIGHT] : NULL,
            ringed ? &saturn_rings : NULL);
    }
    rlDrawRenderBatchActive();
}

/* Phase 4. Translucent layers, after every opaque surface: they test depth but
 * do not write it, so they blend over bodies without hiding each other.
 * Rings and clouds first, then trails (additive: a trail only brightens what
 * lies behind it, so one passing in front of the rings still shows), then the
 * atmospheric halos. */
static void draw_translucent_layers(const FrameContext *ctx)
{
    const SolarSystem *system = ctx->system;
    const RenderResources *resources = ctx->resources;
    rlDisableDepthMask();
    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        if (body->radius_quality == PHYSICAL_UNKNOWN || (int)i == observer_body) continue;
        int faint_rings = ring_system_index(body->id);
        /* Faint rings need their opacity strip: without it they are not drawn
         * (a flat tinted annulus would misrepresent near-transparent dust). */
        bool saturn = body->id == BODY_ID_SATURN || (faint_rings >= 0 && resources->ring_meshes[faint_rings].vertexCount
            && ring_strip(resources, faint_rings, ctx->mode).id);
        bool clouds = body->id == BODY_ID_EARTH && resources->texture_loaded[RENDER_TEXTURE_EARTH_CLOUDS];
        if (!saturn && !clouds) continue;
        if (body_pixels(ctx, i, renderer_body_visual_radius(body, ctx->mode)) < RENDER_MESH_MIN_RADIUS_PIXELS * 2) continue;
        Vector3 position = ctx->position[i];
        float radius = ctx->radius[i];
        RenderOrientation orientation = render_body_orientation(body->id, ctx->view->orientation_days);
        if (saturn) {
            /* Rings lie in Saturn's equatorial plane; seen edge-on or from
             * below they must stay visible, so both faces are drawn. Their
             * outer extent matches renderer_body_visual_radius (framing).
             * Saturn's shadow on them needs only geometry, so it is cast even
             * when the ring map is missing. */
            RingShadow planet_shadow;
            bool ring_map = planet_ring_shadow(resources, body->id, orientation, position, radius, ctx->mode, &planet_shadow);
            Matrix ring_transform = body_matrix(orientation, position, radius);
            rlDisableBackfaceCulling();
            draw_shaded(resources, faint_rings >= 0 ? &resources->ring_meshes[faint_rings] : &resources->ring, &ring_transform, SHADE_RING,
                faint_rings >= 0 ? ring_strip(resources, faint_rings, ctx->mode) : texture_or_white(resources, RENDER_TEXTURE_SATURN_RING),
                ring_map ? WHITE : (Color){205, 184, 145, 150},
                light_toward_star(ctx, i), (RenderAtmosphere){0}, NULL, &planet_shadow);
            rlEnableBackfaceCulling();
        }
        if (clouds) {
            Matrix cloud_transform = body_matrix(orientation, position, radius * 1.012f);
            draw_shaded(resources, &resources->sphere_detailed, &cloud_transform, SHADE_CLOUDS,
                resources->textures[RENDER_TEXTURE_EARTH_CLOUDS], WHITE, light_toward_star(ctx, i), (RenderAtmosphere){0}, NULL, NULL);
        }
    }

    BeginBlendMode(BLEND_ADDITIVE);
    if (ctx->view->show_belt) draw_belt(ctx);
    if (ctx->view->show_orbits) draw_orbit_guides(ctx);
    if (!ctx->view->hide_trails) draw_trails(&ctx->cache, system, ctx->trails, ctx->mode, ctx->trail_frame, ctx->origin, &ctx->camera);
    /* Atmospheric halos: a shell a few percent above each body with air, so
     * the limb glows like Earth seen from orbit. Bodies too small on screen to
     * show a limb are skipped. */
    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        RenderAtmosphere air = render_atmosphere_for_body(body->id);
        if (air.strength <= 0 || body->radius_quality == PHYSICAL_UNKNOWN || (int)i == observer_body) continue;
        if (body_pixels(ctx, i, ctx->radius[i]) < 6.0) continue;
        /* Giant planets have no surface to frame their air against; a softer
         * halo keeps them from looking blurred. */
        if (body->id != BODY_ID_EARTH && body->id != BODY_ID_VENUS && body->id != BODY_ID_MARS && body->id != BODY_ID_TITAN)
            air.strength *= 0.55f;
        Matrix halo = body_matrix(render_body_orientation(body->id, ctx->view->orientation_days), ctx->position[i], ctx->radius[i] * 1.035f);
        draw_shaded(resources, &resources->sphere_detailed, &halo, SHADE_HALO, resources->white, WHITE, light_toward_star(ctx, i), air, NULL, NULL);
    }
    EndBlendMode();
}

/* Phase 5. The Sun's halo, added on top of everything. Far away it keeps a
 * small minimum apparent size so the Sun still reads as a bright star; this
 * is presentation only. */
static void draw_sun_glow(const FrameContext *ctx)
{
    if (ctx->star < 0) return;
    /* Two layers: a tight bright corona and a wide, faint scattered halo
     * (the glare a camera or eye sees around a bright star). */
    /* From Earth (A143) the floor follows the Sun's own distance: the aim
     * point may be a planet many AU away. */
    double reference = observer_body >= 0 ? ctx->distance[ctx->star] : ctx->camera_distance;
    float size = fmaxf(ctx->radius[ctx->star] * 7.0f, (float)reference * 0.05f);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawBillboard(ctx->camera, ctx->resources->glow, ctx->star_position, size, WHITE);
    DrawBillboard(ctx->camera, ctx->resources->glow, ctx->star_position, size * 4.0f, (Color){110, 92, 70, 255});
    EndBlendMode();
}

void renderer_draw_solar_system(const SolarSystem *system, const BodyTrails *trails, RenderScaleMode mode,
    RenderTrailFrame trail_frame, Vec3d origin, const RenderResources *resources, const RenderView *view)
{
    /* Several KB of per-body data: static rather than on the small WASM stack. */
    static FrameContext ctx;
    ctx = (FrameContext){.system = system, .trails = trails, .resources = resources, .view = view, .mode = mode,
        .trail_frame = trail_frame, .origin = origin};
    frame_context_build(&ctx);
    float view_position[3] = {ctx.camera.position.x, ctx.camera.position.y, ctx.camera.position.z};
    if (resources->ready) SetShaderValue(resources->shader, resources->loc_view_pos, view_position, SHADER_UNIFORM_VEC3);

    draw_backdrop(&ctx);
    draw_grid(&ctx);
    draw_opaque_bodies(&ctx);
    if (!resources->ready) {
        /* Flat-colour fallback: trails still draw, just without the
         * translucent layers that need the shader. */
        rlDisableDepthMask();
        draw_trails(&ctx.cache, system, trails, mode, trail_frame, origin, &ctx.camera);
        rlDrawRenderBatchActive();
        rlEnableDepthMask();
        return;
    }
    draw_translucent_layers(&ctx);
    draw_sun_glow(&ctx);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}

int renderer_navigation_neighbor(const SolarSystem *system, size_t index, RenderNavigation direction)
{
    if (index >= system->body_count) return -1;
    int parent = solar_system_parent_index(system, index);
    if (direction == RENDER_NAV_UP) return parent;
    /* Children of `of` in catalog order, skipping unknown-radius markers. */
    int first_child = -1;
    for (size_t i = 0; i < system->body_count && first_child < 0; ++i)
        if (solar_system_parent_index(system, i) == (int)index && system->bodies[i].radius_quality != PHYSICAL_UNKNOWN)
            first_child = (int)i;
    if (direction == RENDER_NAV_DOWN || (parent < 0 && direction == RENDER_NAV_NEXT)) return first_child;
    if (parent < 0) {
        /* Left from a parentless body wraps to its last child. */
        for (size_t i = system->body_count; i-- > 0;)
            if (solar_system_parent_index(system, i) == (int)index && system->bodies[i].radius_quality != PHYSICAL_UNKNOWN)
                return (int)i;
        return -1;
    }
    size_t n = system->body_count;
    for (size_t k = 1; k < n; ++k) {
        size_t i = direction == RENDER_NAV_NEXT ? (index + k) % n : (index + n - k) % n;
        if (solar_system_parent_index(system, i) == parent && system->bodies[i].radius_quality != PHYSICAL_UNKNOWN)
            return (int)i;
    }
    return -1;
}
