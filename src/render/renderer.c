#include "renderer.h"

#include <math.h>
#include <stdlib.h>

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

Vec3d renderer_trail_point_position(const SolarSystem *system, const BodyTrails *trails, size_t body_index, size_t point_index, RenderScaleMode mode)
{
    if (system->body_count == 0 || body_index >= system->body_count || body_index >= SOLAR_SYSTEM_BODY_CAPACITY) {
        return vec3d_zero();
    }

    Vec3d position = meters_vec_to_render_vec3d(body_trails_point_at(trails, body_index, point_index));
    const Body *body = &system->bodies[body_index];
    if (mode == RENDER_SCALE_REAL) {
        return position;
    }

    int parent_index = solar_system_parent_index(system, body_index);
    if (parent_index < 0 || point_index >= body_trails_point_count(trails, (size_t)parent_index)) {
        return position;
    }

    const Body *parent = &system->bodies[parent_index];
    if (body->kind != BODY_KIND_MOON && parent->kind == BODY_KIND_STAR) return position;
    Vec3d parent_position = meters_vec_to_render_vec3d(body_trails_point_at(trails, (size_t)parent_index, point_index));
    return visible_satellite_position(body, parent, position, parent_position, mode);
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

typedef enum ShadeMode { SHADE_LIT = 0, SHADE_STAR = 1, SHADE_CLOUDS = 2, SHADE_RING = 3, SHADE_SKY = 4 } ShadeMode;

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
    Material material = resources->material;
    material.maps[MATERIAL_MAP_ALBEDO].texture = surface;
    material.maps[MATERIAL_MAP_ALBEDO].color = tint;
    material.maps[MATERIAL_MAP_METALNESS].texture = night ? *night : resources->white;
    material.maps[MATERIAL_MAP_NORMAL].texture = rings ? rings->ring_opacity : resources->white;
    DrawMesh(*mesh, material, *transform);
}

/* Ring geometry for the shadow test, matching the ring mesh (radii in planet
 * radii, plane normal = Saturn's pole). Needs the ring opacity map. */
static bool saturn_ring_shadow(const RenderResources *resources, RenderOrientation orientation, Vector3 position,
    float radius, RingShadow *out)
{
    if (!resources->texture_loaded[RENDER_TEXTURE_SATURN_RING]) return false;
    *out = (RingShadow){position, {(float)orientation.pole.x, (float)orientation.pole.y, (float)orientation.pole.z},
        radius, (float)SOLAR_SATURN_RING_VISUAL_INNER_RATIO, (float)(SOLAR_SATURN_RING_OUTER_RADIUS_M / SOLAR_SATURN_RADIUS_M),
        resources->textures[RENDER_TEXTURE_SATURN_RING]};
    return true;
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

static void draw_trails(const SolarSystem *system, const BodyTrails *trails, RenderScaleMode mode,
    RenderTrailFrame trail_frame, Vec3d origin)
{
    rlBegin(RL_LINES);
    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        if (body->kind == BODY_KIND_STAR && body->fixed) continue;
        size_t point_count = body_trails_point_count(trails, i);
        if (point_count < 2) continue;
        Color color = renderer_body_color(body);
        color.a = 235;
        size_t stride = renderer_trail_sample_stride(point_count);
        /* Adjacent segments share an endpoint. Reuse its render transform;
         * the simulation and synchronized history are immutable while drawing.
         * Opacity follows the sample's age so recent motion reads first. */
        size_t previous = 0;
        Vec3d start = renderer_trail_point_in_frame(system, trails, i, 0, mode, trail_frame);
        for (size_t j = stride; j < point_count; j += stride) {
            Vec3d end = renderer_trail_point_in_frame(system, trails, i, j, mode, trail_frame);
            line_vertex(renderer_relative_vector(start, origin), color, render_trail_alpha(previous, point_count));
            line_vertex(renderer_relative_vector(end, origin), color, render_trail_alpha(j, point_count));
            start = end;
            previous = j;
        }
        if (previous + 1 < point_count) {
            Vec3d end = renderer_trail_point_in_frame(system, trails, i, point_count - 1, mode, trail_frame);
            line_vertex(renderer_relative_vector(start, origin), color, render_trail_alpha(previous, point_count));
            line_vertex(renderer_relative_vector(end, origin), color, 1.0f);
        }
    }
    rlEnd();
}

void renderer_draw_solar_system(const SolarSystem *system, const BodyTrails *trails, RenderScaleMode mode,
    RenderTrailFrame trail_frame, Vec3d origin, const RenderResources *resources, const RenderView *view)
{
    Camera3D camera = view->camera;
    Vector3 to_camera = {camera.position.x - camera.target.x, camera.position.y - camera.target.y, camera.position.z - camera.target.z};
    double camera_distance = sqrt(to_camera.x * to_camera.x + to_camera.y * to_camera.y + to_camera.z * to_camera.z);
    float view_position[3] = {camera.position.x, camera.position.y, camera.position.z};
    if (resources->ready) SetShaderValue(resources->shader, resources->loc_view_pos, view_position, SHADER_UNIFORM_VEC3);

    /* 1. Milky Way backdrop: a huge inside-out sphere centred on the camera,
     * drawn first without writing depth, so everything else draws over it and
     * it never moves as the camera translates (stars are effectively at
     * infinity). Illustrative, not a sky ephemeris. */
    if (resources->ready && resources->texture_loaded[RENDER_TEXTURE_STARS]) {
        rlDrawRenderBatchActive();
        rlDisableDepthMask();
        rlDisableBackfaceCulling();
        Matrix sky = body_matrix((RenderOrientation){{0, 1, 0}, {1, 0, 0}, 0, false}, camera.position, view->far_plane * 0.5f);
        draw_shaded(resources, &resources->sphere_detailed, &sky, SHADE_SKY, resources->textures[RENDER_TEXTURE_STARS],
            WHITE, (Vector3){0, 1, 0}, (RenderAtmosphere){0}, NULL, NULL);
        rlEnableBackfaceCulling();
        rlEnableDepthMask();
    }

    /* 2. Reference grid: it writes no depth, so every body drawn later covers
     * it even where the plane passes through the body's centre. The grid
     * reads as a background reference rather than slicing planets in half. */
    rlDrawRenderBatchActive();
    rlDisableDepthMask();
    draw_reference_grid(origin, camera_distance);
    rlDrawRenderBatchActive();
    rlEnableDepthMask();


    /* The light source is the first star; lessons without one (the two-sphere
     * contact demo) light each body from the camera instead. */
    int star = -1;
    for (size_t i = 0; i < system->body_count && star < 0; ++i) if (system->bodies[i].kind == BODY_KIND_STAR) star = (int)i;
    Vector3 star_position = star >= 0 ? renderer_relative_vector(renderer_body_position(system, (size_t)star, mode), origin) : camera.position;

    /* 3. Opaque bodies. Each mesh draw costs dozens of WebGL calls, so bodies
     * behind the camera are skipped and sub-pixel bodies become one batched
     * point (presentation only: their SI state is untouched). */
    Vector3 forward = unit_vector((Vector3){-to_camera.x, -to_camera.y, -to_camera.z}, (Vector3){0, 0, -1});
    double viewport_height = (double)GetRenderHeight();
    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        Vector3 position = renderer_relative_vector(renderer_body_position(system, i, mode), origin);
        float radius = renderer_body_radius(body, mode);
        Vector3 offset = {position.x - camera.position.x, position.y - camera.position.y, position.z - camera.position.z};
        double depth = offset.x * forward.x + offset.y * forward.y + offset.z * forward.z;
        if (depth < -(double)renderer_body_visual_radius(body, mode)) continue;
        double pixels = render_projected_radius_pixels(radius,
            sqrt(offset.x * offset.x + offset.y * offset.y + offset.z * offset.z), camera.fovy, viewport_height);
        if (resources->ready && body->kind != BODY_KIND_STAR && pixels < RENDER_MESH_MIN_RADIUS_PIXELS) {
            DrawPoint3D(position, renderer_body_color(body));
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
        RenderOrientation orientation = render_body_orientation(body->id, view->orientation_days);
        Matrix transform = body_matrix(orientation, position, radius);
        const Mesh *mesh = textured || body->kind != BODY_KIND_MOON ? &resources->sphere_detailed : &resources->sphere_simple;
        Color tint = textured ? WHITE : renderer_body_color(body);
        if ((int)i == star) {
            draw_shaded(resources, mesh, &transform, SHADE_STAR, texture_or_white(resources, slot), tint,
                (Vector3){0, 1, 0}, (RenderAtmosphere){0}, NULL, NULL);
            continue;
        }
        Vector3 light = unit_vector((Vector3){star_position.x - position.x, star_position.y - position.y,
            star_position.z - position.z}, (Vector3){0, 1, 0});
        bool city_lights = body->id == BODY_ID_EARTH && resources->texture_loaded[RENDER_TEXTURE_EARTH_NIGHT];
        RingShadow saturn_rings;
        bool ringed = body->id == BODY_ID_SATURN && saturn_ring_shadow(resources, orientation, position, radius, &saturn_rings);
        draw_shaded(resources, mesh, &transform, SHADE_LIT, texture_or_white(resources, slot), tint, light,
            render_atmosphere_for_body(body->id), city_lights ? &resources->textures[RENDER_TEXTURE_EARTH_NIGHT] : NULL,
            ringed ? &saturn_rings : NULL);
    }
    rlDrawRenderBatchActive();
    if (!resources->ready) {
        /* Flat-colour fallback: trails still draw, just without the
         * translucent layers that need the shader. */
        rlDisableDepthMask();
        draw_trails(system, trails, mode, trail_frame, origin);
        rlDrawRenderBatchActive();
        rlEnableDepthMask();
        return;
    }

    /* 4. Translucent layers after every opaque surface, testing depth but not
     * writing it, so they blend over bodies without hiding each other. Trails
     * come first: if they wrote depth before the planets were drawn, even a
     * faint, nearly transparent old segment would punch a dark line through
     * any planet behind it. Additive blending lets a trail only brighten what
     * lies behind it. */
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ADDITIVE);
    draw_trails(system, trails, mode, trail_frame, origin);
    EndBlendMode();
    for (size_t i = 0; i < system->body_count; ++i) {
        const Body *body = &system->bodies[i];
        if (body->radius_quality == PHYSICAL_UNKNOWN) continue;
        bool saturn = body->id == BODY_ID_SATURN;
        bool clouds = body->id == BODY_ID_EARTH && resources->texture_loaded[RENDER_TEXTURE_EARTH_CLOUDS];
        if (!saturn && !clouds) continue;
        Vector3 position = renderer_relative_vector(renderer_body_position(system, i, mode), origin);
        float radius = renderer_body_radius(body, mode);
        Vector3 offset = {position.x - camera.position.x, position.y - camera.position.y, position.z - camera.position.z};
        double distance = sqrt(offset.x * offset.x + offset.y * offset.y + offset.z * offset.z);
        if (render_projected_radius_pixels(renderer_body_visual_radius(body, mode), distance, camera.fovy, viewport_height)
            < RENDER_MESH_MIN_RADIUS_PIXELS * 2) continue;
        Vector3 light = unit_vector((Vector3){star_position.x - position.x, star_position.y - position.y,
            star_position.z - position.z}, (Vector3){0, 1, 0});
        RenderOrientation orientation = render_body_orientation(body->id, view->orientation_days);
        if (saturn) {
            /* Rings lie in Saturn's equatorial plane; seen edge-on or from
             * below they must stay visible, so both faces are drawn. Their
             * outer extent matches renderer_body_visual_radius (framing). */
            bool ring_map = resources->texture_loaded[RENDER_TEXTURE_SATURN_RING];
            rlDisableBackfaceCulling();
            Matrix ring_transform = body_matrix(orientation, position, radius);
            RingShadow saturn_rings;
            bool shadowed = saturn_ring_shadow(resources, orientation, position, radius, &saturn_rings);
            draw_shaded(resources, &resources->ring, &ring_transform, SHADE_RING,
                texture_or_white(resources, RENDER_TEXTURE_SATURN_RING), ring_map ? WHITE : (Color){205, 184, 145, 150},
                light, (RenderAtmosphere){0}, NULL, shadowed ? &saturn_rings : NULL);
            rlEnableBackfaceCulling();
        }
        if (clouds) {
            Matrix cloud_transform = body_matrix(orientation, position, radius * 1.012f);
            draw_shaded(resources, &resources->sphere_detailed, &cloud_transform,
                SHADE_CLOUDS, resources->textures[RENDER_TEXTURE_EARTH_CLOUDS], WHITE, light, (RenderAtmosphere){0}, NULL, NULL);
        }
    }

    /* 5. The Sun's halo, added on top of everything (additive blending). Far
     * away it keeps a small minimum apparent size so the Sun still reads as a
     * bright star; this is presentation only. */
    if (star >= 0) {
        float sun_radius = renderer_body_radius(&system->bodies[star], mode);
        float size = fmaxf(sun_radius * 7.0f, (float)camera_distance * 0.05f);
        BeginBlendMode(BLEND_ADDITIVE);
        DrawBillboard(camera, resources->glow, star_position, size, WHITE);
        EndBlendMode();
    }
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
}
