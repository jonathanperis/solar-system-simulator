#include "renderer.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <rlgl.h>

#include "../sim/constants.h"
#include "render_scale.h"

#define SOLAR_ILLUSTRATIVE_SATELLITE_GAP_UNITS 0.03
#define SOLAR_ILLUSTRATIVE_SMALL_MOON_RADIUS 0.012f

Vector3 renderer_relative_vector(Vec3d position, Vec3d origin)
{
    Vec3d vector = vec3d_sub(position, origin);
    return (Vector3){
        (float)vector.x,
        (float)vector.y,
        (float)vector.z,
    };
}

float renderer_body_radius(const Body *body, RenderScaleMode mode)
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

    return SOLAR_ILLUSTRATIVE_PLANET_RADIUS;
}

float renderer_body_visual_radius(const Body *body, RenderScaleMode mode)
{
    float body_radius = renderer_body_radius(body, mode);
    if (body->id != BODY_ID_SATURN) return body_radius;

    if (mode == RENDER_SCALE_REAL) {
        return meters_to_render_units(SOLAR_SATURN_RING_OUTER_RADIUS_M);
    }
    return body_radius * (float)(SOLAR_SATURN_RING_OUTER_RADIUS_M / SOLAR_SATURN_RADIUS_M);
}

static Vec3d visible_satellite_position(const Body *body, const Body *parent, Vec3d position, Vec3d parent_position, RenderScaleMode mode)
{
    Vec3d relative_position = vec3d_sub(position, parent_position);
    double current_distance = vec3d_length(relative_position);
    if (current_distance <= 0.0) {
        return position;
    }

    double required_distance =
        renderer_body_radius(parent, mode) +
        renderer_body_radius(body, mode) +
        SOLAR_ILLUSTRATIVE_SATELLITE_GAP_UNITS;

    if (body->id == BODY_ID_MOON) {
        Vec3d expanded = vec3d_scale(relative_position, SOLAR_ILLUSTRATIVE_MOON_DISTANCE_FACTOR);
        if (vec3d_length(expanded) > required_distance) {
            return vec3d_add(parent_position, expanded);
        }
    }

    if (current_distance < required_distance) {
        return vec3d_add(parent_position, vec3d_scale(relative_position, required_distance / current_distance));
    }

    return position;
}

Vec3d renderer_body_position(const SolarSystem *system, size_t body_index, RenderScaleMode mode)
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
static Vec3d trail_point_with_parent(const SolarSystem *system, const BodyTrails *trails, size_t body_index,
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
        : (double)renderer_body_radius(body, mode) / meters_to_render_units(body->radius_m);
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
static bool saturn_ring_shadow(const RenderResources *resources, RenderOrientation orientation, Vector3 position,
    float radius, RingShadow *out)
{
    bool ring_map = resources->texture_loaded[RENDER_TEXTURE_SATURN_RING];
    *out = (RingShadow){
        .center = position,
        .ring_normal = {(float)orientation.pole.x, (float)orientation.pole.y, (float)orientation.pole.z},
        .radius = radius,
        .ring_inner = (float)SOLAR_SATURN_RING_VISUAL_INNER_RATIO,
        .ring_outer = (float)(SOLAR_SATURN_RING_OUTER_RADIUS_M / SOLAR_SATURN_RADIUS_M),
        .ring_opacity = ring_map ? resources->textures[RENDER_TEXTURE_SATURN_RING] : resources->white,
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

static void line_vertex(Vector3 position, Color color, float alpha)
{
    rlColor4ub(color.r, color.g, color.b, (unsigned char)(color.a * (alpha < 0 ? 0 : alpha > 1 ? 1 : alpha)));
    rlVertex3f(position.x, position.y, position.z);
}

/* Adaptive ecliptic-plane reference grid (A67): power-of-ten minor and major
 * lines that cross-fade with zoom, each line split into short pieces so its
 * opacity can fall off with distance from the camera target. */
static void draw_reference_grid(Vec3d origin, double camera_distance)
{
    RenderGridLevels grid = render_grid_levels(camera_distance);
    const int pieces = 6;
    const Color minor_color = {70, 92, 118, 70}, major_color = {92, 120, 150, 120};
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
                    line_vertex(renderer_relative_vector(a, origin), color, (float)(strength * render_grid_alpha(da, grid.radius) * 2.0));
                    line_vertex(renderer_relative_vector(b, origin), color, (float)(strength * render_grid_alpha(db, grid.radius) * 2.0));
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
static void draw_trails(const RenderFrameCache *cache, const SolarSystem *system, const BodyTrails *trails,
    RenderScaleMode mode, RenderTrailFrame trail_frame, Vec3d origin, const Camera3D *camera)
{
    Vec3d camera_world = vec3d_add(origin, (Vec3d){camera->position.x, camera->position.y, camera->position.z});
    double viewport_height = (double)GetRenderHeight();
    rlBegin(RL_LINES);
    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        if (body->kind == BODY_KIND_STAR && body->fixed) continue;
        size_t point_count = body_trails_point_count(trails, i);
        if (point_count < 2) continue;
        Color color = renderer_body_color(body);
        color.a = 235;
        /* Draw only as finely as the trail's on-screen length can show. */
        size_t stride = render_trail_stride_for_extent(point_count, renderer_trail_sample_stride(point_count),
            trail_path_pixels(cache, system, trails, i, point_count, mode, trail_frame, camera_world, camera, viewport_height));
        /* The live endpoint is the body's centre: stop the trail at its drawn
         * surface so it never pokes out through the near side. */
        Vec3d center = cache->position[i];
        double surface = body->radius_quality == PHYSICAL_UNKNOWN ? 0 : renderer_body_radius(body, mode);
        /* Adjacent segments share an endpoint. Reuse its render transform;
         * the simulation and synchronized history are immutable while drawing.
         * Opacity follows the sample's age so recent motion reads first. */
        size_t previous = 0;
        Vec3d start = renderer_trail_point_cached(cache, system, trails, i, 0, mode, trail_frame);
        for (size_t j = stride; j < point_count; j += stride) {
            Vec3d end = renderer_trail_point_cached(cache, system, trails, i, j, mode, trail_frame);
            Vec3d a = start, b = end;
            if (render_clip_segment_outside_sphere(&a, &b, center, surface)) {
                line_vertex(renderer_relative_vector(a, origin), color, render_trail_alpha(previous, point_count));
                line_vertex(renderer_relative_vector(b, origin), color, render_trail_alpha(j, point_count));
            }
            start = end;
            previous = j;
        }
        if (previous + 1 < point_count) {
            Vec3d end = renderer_trail_point_cached(cache, system, trails, i, point_count - 1, mode, trail_frame);
            Vec3d a = start, b = end;
            if (render_clip_segment_outside_sphere(&a, &b, center, surface)) {
                line_vertex(renderer_relative_vector(a, origin), color, render_trail_alpha(previous, point_count));
                line_vertex(renderer_relative_vector(b, origin), color, 1.0f);
            }
        }
    }
    rlEnd();
}

void renderer_draw_labels(const SolarSystem *system, RenderScaleMode mode, Vec3d origin,
    const RenderResources *resources, Camera3D camera, size_t selected)
{
    static RenderLabelBox boxes[SOLAR_SYSTEM_BODY_CAPACITY];
    static size_t body_of[SOLAR_SYSTEM_BODY_CAPACITY];
    static Vector2 screen[SOLAR_SYSTEM_BODY_CAPACITY];
    static bool on_screen[SOLAR_SYSTEM_BODY_CAPACITY], in_front[SOLAR_SYSTEM_BODY_CAPACITY];
    static double occluder_distance[SOLAR_SYSTEM_BODY_CAPACITY], occluder_radius[SOLAR_SYSTEM_BODY_CAPACITY];
    const float size = 15.0f, spacing = 0.5f;
    Font font = resources->label_font_ready ? resources->label_font : GetFontDefault();
    Vector3 forward = unit_vector((Vector3){camera.target.x - camera.position.x, camera.target.y - camera.position.y,
        camera.target.z - camera.position.z}, (Vector3){0, 0, -1});
    float width = (float)GetScreenWidth(), height = (float)GetScreenHeight();
    double viewport_height = (double)height;

    /* Project every body once; labels then only read the cached results. */
    for (size_t i = 0; i < system->body_count && i < SOLAR_SYSTEM_BODY_CAPACITY; ++i) {
        Vector3 position = renderer_relative_vector(renderer_body_position(system, i, mode), origin);
        Vector3 offset = {position.x - camera.position.x, position.y - camera.position.y, position.z - camera.position.z};
        in_front[i] = offset.x * forward.x + offset.y * forward.y + offset.z * forward.z > 0;
        screen[i] = in_front[i] ? GetWorldToScreen(position, camera) : (Vector2){-1e6f, -1e6f};
        on_screen[i] = in_front[i] && screen[i].x >= 0 && screen[i].x <= width && screen[i].y >= 0 && screen[i].y <= height;
        occluder_distance[i] = sqrt(offset.x * offset.x + offset.y * offset.y + offset.z * offset.z);
        occluder_radius[i] = system->bodies[i].radius_quality == PHYSICAL_UNKNOWN ? 0 : render_projected_radius_pixels(
            renderer_body_radius(&system->bodies[i], mode), occluder_distance[i], camera.fovy, viewport_height);
    }

    size_t count = 0;
    for (size_t i = 0; i < system->body_count && i < SOLAR_SYSTEM_BODY_CAPACITY; ++i) {
        if (!on_screen[i]) continue;
        const Body *body = &system->bodies[i];
        Vector3 position = renderer_relative_vector(renderer_body_position(system, i, mode), origin);
        Vector3 offset = {position.x - camera.position.x, position.y - camera.position.y, position.z - camera.position.z};
        double distance = sqrt(offset.x * offset.x + offset.y * offset.y + offset.z * offset.z);
        double radius_px = render_projected_radius_pixels(renderer_body_radius(body, mode), distance, camera.fovy, viewport_height);
        int parent = solar_system_parent_index(system, i);
        double separation = 1e9;
        if (parent >= 0 && system->bodies[parent].kind != BODY_KIND_STAR)
            separation = hypot(screen[i].x - screen[parent].x, screen[i].y - screen[parent].y);
        if (!render_body_wants_label(body->kind, i == selected, body->radius_quality != PHYSICAL_UNKNOWN, radius_px, separation))
            continue;
        /* Hidden behind a nearer body that is large on screen: no label. */
        bool hidden = false;
        for (size_t j = 0; j < system->body_count && !hidden; ++j) {
            /* A large nearer body can hide a label even when its own centre
             * is just off screen, so occluders only need to be in front. */
            if (j == i || !in_front[j] || occluder_radius[j] < 8.0 || occluder_distance[j] >= distance) continue;
            hidden = hypot(screen[i].x - screen[j].x, screen[i].y - screen[j].y) < occluder_radius[j];
        }
        if (hidden) continue;
        Vector2 text = MeasureTextEx(font, body->name, size, spacing);
        int priority = i == selected ? 1000 : body->kind == BODY_KIND_STAR ? 900
            : body->kind == BODY_KIND_PLANET ? 800 - (int)i : 400 - (int)i;
        /* Above the body, but kept on screen when the body fills the view. */
        float top = fmaxf(4.0f, screen[i].y - (float)radius_px - 6 - text.y);
        boxes[count] = (RenderLabelBox){screen[i].x - text.x / 2, top, text.x, text.y, priority, false};
        body_of[count++] = i;
    }
    render_declutter_labels(boxes, count);
    for (size_t k = 0; k < count; ++k) {
        if (!boxes[k].visible) continue;
        const Body *body = &system->bodies[body_of[k]];
        Color color = body_of[k] == selected ? (Color){240, 200, 120, 255} : (Color){228, 222, 206, 225};
        /* A soft drop shadow keeps pale text readable over bright planets. */
        DrawTextEx(font, body->name, (Vector2){boxes[k].x + 1, boxes[k].y + 1}, size, spacing, (Color){0, 0, 0, 170});
        DrawTextEx(font, body->name, (Vector2){boxes[k].x, boxes[k].y}, size, spacing, color);
    }
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
    ctx->viewport_height = (double)GetRenderHeight();
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

/* Phase 1. Milky Way backdrop: a huge inside-out sphere centred on the camera,
 * drawn first without writing depth, so everything else draws over it and it
 * never moves as the camera translates (stars are effectively at infinity).
 * Illustrative, not a sky ephemeris. */
static void draw_backdrop(const FrameContext *ctx)
{
    const RenderResources *resources = ctx->resources;
    if (!resources->ready || !resources->texture_loaded[RENDER_TEXTURE_STARS]) return;
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    rlDisableBackfaceCulling();
    RenderOrientation ecliptic = {.pole = {0, 1, 0}, .prime_meridian = {1, 0, 0}, .spin_rate_degrees_per_day = 0, .known = false};
    Matrix sky = body_matrix(ecliptic, ctx->camera.position, ctx->view->far_plane * 0.5f);
    draw_shaded(resources, &resources->sphere_detailed, &sky, SHADE_SKY, resources->textures[RENDER_TEXTURE_STARS],
        WHITE, (Vector3){0, 1, 0}, (RenderAtmosphere){0}, NULL, NULL);
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
    if (ctx->view->show_grid) draw_reference_grid(ctx->origin, ctx->camera_distance);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
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
        if (body->radius_quality == PHYSICAL_UNKNOWN || !resources->ready) {
            /* Unknown radius: an explicitly nonphysical wire marker (V25). */
            if (body->radius_quality == PHYSICAL_UNKNOWN) DrawSphereWires(position, radius, 4, 4, renderer_body_color(body));
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
        /* The rings' shadow on Saturn needs the ring opacity map. */
        RingShadow saturn_rings;
        bool ringed = body->id == BODY_ID_SATURN && saturn_ring_shadow(resources, orientation, position, radius, &saturn_rings);
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
        if (body->radius_quality == PHYSICAL_UNKNOWN) continue;
        bool saturn = body->id == BODY_ID_SATURN;
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
            bool ring_map = resources->texture_loaded[RENDER_TEXTURE_SATURN_RING];
            RingShadow planet_shadow;
            saturn_ring_shadow(resources, orientation, position, radius, &planet_shadow);
            Matrix ring_transform = body_matrix(orientation, position, radius);
            rlDisableBackfaceCulling();
            draw_shaded(resources, &resources->ring, &ring_transform, SHADE_RING,
                texture_or_white(resources, RENDER_TEXTURE_SATURN_RING), ring_map ? WHITE : (Color){205, 184, 145, 150},
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
    draw_trails(&ctx->cache, system, ctx->trails, ctx->mode, ctx->trail_frame, ctx->origin, &ctx->camera);
    /* Atmospheric halos: a shell a few percent above each body with air, so
     * the limb glows like Earth seen from orbit. Bodies too small on screen to
     * show a limb are skipped. */
    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        RenderAtmosphere air = render_atmosphere_for_body(body->id);
        if (air.strength <= 0 || body->radius_quality == PHYSICAL_UNKNOWN) continue;
        if (body_pixels(ctx, i, ctx->radius[i]) < 6.0) continue;
        /* Giant planets have no surface to frame their air against; a softer
         * halo keeps them from looking blurred. */
        if (body->id != BODY_ID_EARTH && body->id != BODY_ID_VENUS && body->id != BODY_ID_MARS) air.strength *= 0.55f;
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
    float size = fmaxf(ctx->radius[ctx->star] * 7.0f, (float)ctx->camera_distance * 0.05f);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawBillboard(ctx->camera, ctx->resources->glow, ctx->star_position, size, WHITE);
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
